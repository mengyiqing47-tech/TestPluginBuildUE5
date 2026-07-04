// Fill out your copyright notice in the Description page of Project Settings.


#include "CWeatherBPFLibrary.h" // 本类头文件（含 FMMoonData/FMSunData 结构体和函数声明）
#include "Kismet/KismetMathLibrary.h" // UKismetMathLibrary 数学工具（三角函数、闰年判断等）
#include "CWeatherSystem.h" // PWeatherSys 命名空间（全局天气状态变量）
#include "SunPosition.h" // USunPositionFunctionLibrary 太阳位置计算库

// --- 全局天气状态 Getter/Setter ---

void UCWeatherBPFLibrary::SetNetSyncTime(int32 InSyncTime)
{
	PWeatherSys::GWeatherInfo.NetSyncTime = InSyncTime; // 将网络同步时间写入全局天气信息结构体
}

int32 UCWeatherBPFLibrary::GetNetSyncTime()
{
	return PWeatherSys::GWeatherInfo.NetSyncTime; // 读取全局天气信息中的网络同步时间
}

void UCWeatherBPFLibrary::SetGSkyAtmosphereInfo(const FSkyAtmosphereInfo& InSkyAtmosphereInfo)
{
	PWeatherSys::GSkyAtmosphereInfo = InSkyAtmosphereInfo; // 更新全局天空大气配置信息
}

FSkyAtmosphereInfo UCWeatherBPFLibrary::GetGSkyAtmosphereInfo()
{
	return PWeatherSys::GSkyAtmosphereInfo; // 读取全局天空大气配置
}

void UCWeatherBPFLibrary::SetGCloudStatInfo(const FCloudStatInfo& InCloudInfo)
{
	PWeatherSys::GCloudStatInfo = InCloudInfo; // 更新全局云状态信息
}

FCloudStatInfo UCWeatherBPFLibrary::GetGCloudStatInfo()
{
	return PWeatherSys::GCloudStatInfo; // 读取全局云状态
}

void UCWeatherBPFLibrary::SetGWeatherInfo(const FWeatherInfo& InWeatherInfo)
{
	PWeatherSys::GWeatherInfo = InWeatherInfo; // 更新全局天气信息（含闪电队列和模拟时间）
}

FWeatherInfo UCWeatherBPFLibrary::GetGWeatherInfo()
{
	return PWeatherSys::GWeatherInfo; // 读取全局天气信息
}

// --- 日期计算 ---

// 输入日期 + N 天后，计算新的年月日（支持跨月、跨年、闰年）
void UCWeatherBPFLibrary::CalcDateAddN(int32 InYear, int32 InMonth, int32 InDay, int32 AddNDay, int32& OutYear, int32& OutMonth, int32& OutDay)
{
	if (AddNDay <= 0) // 不增加天数则直接返回原日期
	{
		OutYear = InYear;
		OutMonth = InMonth;
		OutDay = InDay;
		return;
	}
	// 判断当年是否为闰年，调整二月天数
	int32 MonthDays[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; // 每月天数表（索引 1~12）
	bool bIsLeapYear = UKismetMathLibrary::IsLeapYear(InYear); // 调用引擎闰年判断函数
	if (bIsLeapYear)
		MonthDays[2] = 29; // 闰年二月 29 天
	else
		MonthDays[2] = 28; // 平年二月 28 天

	int32 RestNumDay = 0, NumDay = 0; // 年内剩余天数、当前是一年中第几天

	for (int32 i = 1; i < InMonth; i++) // 累加前几月的总天数
	{
		NumDay += MonthDays[i];
	}
	NumDay += InDay; // 加上当前月天数 = 一年中第几天

	if (bIsLeapYear)
		RestNumDay = 367 - NumDay; // 闰年剩余天数
	else
		RestNumDay = 366 - NumDay; // 平年剩余天数

	if (AddNDay >= RestNumDay) // 如果要增加的天数超出年内剩余 → 跨年递归
	{
		AddNDay = AddNDay - RestNumDay; // 减去今年剩余天数
		CalcDateAddN(InYear + 1, 1, 1, AddNDay, OutYear, OutMonth, OutDay); // 递归进入下一年
	}
	else // 不跨年：在当前年内推算
	{
		OutYear = InYear;
		OutMonth = InMonth;
		OutDay = InDay;

		while (AddNDay > 0) // 逐月消耗剩余天数
		{
			int32 RestDay = MonthDays[OutMonth] - OutDay + 1; // 当月剩余天数（含当天）
			if (AddNDay >= RestDay) // 超出当月 → 进入下一个月
			{
				AddNDay = AddNDay - RestDay;
				OutMonth++;
				OutDay = 1;
			}
			else // 当月内就能消化
			{
				OutDay += AddNDay;
				AddNDay = 0;
			}
		}
	}
}

// --- 天气标识位检查 ---

// 检查天气帧有效性标识中是否置位了指定标记
bool UCWeatherBPFLibrary::CheckUEWeatherValidTag(int32 InVal, UE_WEATHER_VALID_TAG InCheckTag)
{
	return InVal & (1<<(int32)InCheckTag); // 左移生成掩码，按位与检查
}


// --- 时间转换 ---

// 将太阳时（浮点小时，如 6.5 = 6:30）分解为时/分/秒
void UCWeatherBPFLibrary::PGetHMSFromSolarTime(float SolarTime, int32& Hour, int32& Minute, int32& Second)
{
	SolarTime = int(SolarTime) % 24 + FMath::Frac(SolarTime) * (SolarTime > 0 ? 1 : -1); // 限制在 [0, 24) 范围，保持小数部分符号正确
	double dSolarTime = SolarTime; // 转为双精度保证精度
	Hour = int(dSolarTime); // 整数部分 = 小时
	Minute = int(dSolarTime * 60) % 60; // 分钟
	Second = int(dSolarTime * 3600) % 60; // 秒
}

// ============================================================
// 天文算法辅助函数
// 基于 JP Meeus 天文算法 (Astronomical Algorithms)
// 用于计算太阳、月亮、恒星的精确位置
// ============================================================

// 计算儒略日 (Julian Day)
// 将格里高利历日期转换为连续的儒略日数
double getJD(int32 Day, int32 Month, int32 Year)
{
	double Y = double(Year); // 年
	double M = double(Month); // 月
	double D = double(Day);   // 日
	if (M == 1.0 || M == 2.0) // 1 月或 2 月视为上一年的 13 月或 14 月
	{
		Y = Y - 1.0;
		M = M + 12.0;
	}
	double A = int(Y / 100.0); // 世纪修正因子
	double B = 2.0 - A + int(A / 4.0); // 格里高利历闰年修正
	double JD = int(365.25 * (Y + 4716.0)) + int(30.6001 * (M + 1.0)) + D + B - 1524.5; // 儒略日标准公式
	return JD;
}


// 月球经度周期项求和
// 使用 60 项三角级数的展开计算月球视经度周期项
// D=日月平均距角, M=太阳平近点角, M1=月球平近点角, F=月球升交点距角, E=轨道偏心率因子
// A1/A2=金星摄动项, L1=月球平经度
double getSumOfPeriodicTermsOfLongitude(double D, double M, double M1, double F, double E, double A1, double A2, double L1)
{
	double l = 0.0;
	l += 3958.0 * FMath::Sin(FMath::DegreesToRadians(A1));    // 金星摄动项
	l += 1962.0 * FMath::Sin(FMath::DegreesToRadians(L1 - F));  // 月球升交点修正
	l += 318.0 * FMath::Sin(FMath::DegreesToRadians(A2));       // 金星摄动项2
	// 以下为月球视经度的 60 项周期展开（来自 ELP-2000/82 简化天文历表）
	l += 6288774.0 * FMath::Sin(FMath::DegreesToRadians(M1));
	l += 1274027.0 * FMath::Sin(FMath::DegreesToRadians(2 * D - M1));
	l += 658314.0 * FMath::Sin(FMath::DegreesToRadians(2 * D));
	l += 213618.0 * FMath::Sin(FMath::DegreesToRadians(2 * M1));
	l += -185116.0 * E * FMath::Sin(FMath::DegreesToRadians(M));
	l += -114332.0 * FMath::Sin(FMath::DegreesToRadians(2 * F));
	l += 58793.0 * FMath::Sin(FMath::DegreesToRadians(2 * D - 2 * M1));
	l += 57066.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D - M - M1));
	l += 53322.0 * FMath::Sin(FMath::DegreesToRadians(2 * D + M1));
	l += 45758.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D - M));
	l += -40923.0 * E * FMath::Sin(FMath::DegreesToRadians(M - M1));
	l += -34720.0 * FMath::Sin(FMath::DegreesToRadians(D));
	l += -30383.0 * E * FMath::Sin(FMath::DegreesToRadians(M + M1));
	l += 15327.0 * FMath::Sin(FMath::DegreesToRadians(2 * D - 2 * F));
	l += -12528.0 * FMath::Sin(FMath::DegreesToRadians(M1 + 2 * F));
	l += 10980.0 * FMath::Sin(FMath::DegreesToRadians(M1 - 2 * F));
	l += 10675.0 * FMath::Sin(FMath::DegreesToRadians(4 * D - M1));
	l += 10034.0 * FMath::Sin(FMath::DegreesToRadians(3 * M1));
	l += 8548.0 * FMath::Sin(FMath::DegreesToRadians(4 * D - 2 * M1));
	l += -7888.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D + M - M1));
	l += -6766.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D + M));
	l += -5163.0 * FMath::Sin(FMath::DegreesToRadians(D - M1));
	l += 4987.0 * E * FMath::Sin(FMath::DegreesToRadians(D + M));
	l += 4036.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D - M + M1));
	l += 3994.0 * FMath::Sin(FMath::DegreesToRadians(2 * D + 2 * M1));
	l += 3861.0 * FMath::Sin(FMath::DegreesToRadians(4 * D));
	l += 3665.0 * FMath::Sin(FMath::DegreesToRadians(2 * D - 3 * M1));
	l += -2689.0 * E * FMath::Sin(FMath::DegreesToRadians(M - 2 * M1));
	l += -2602.0 * FMath::Sin(FMath::DegreesToRadians(2 * D - M1 + 2 * F));
	l += 2390.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D - M - 2 * M1));
	l += -2348.0 * FMath::Sin(FMath::DegreesToRadians(D + M1));
	l += 2236.0 * E * E * FMath::Sin(FMath::DegreesToRadians(2 * D - 2 * M));
	l += -2120.0 * E * FMath::Sin(FMath::DegreesToRadians(M + 2 * M1));
	l += -2069.0 * E * E * FMath::Sin(FMath::DegreesToRadians(2 * M));
	l += 2048.0 * E * E * FMath::Sin(FMath::DegreesToRadians(2 * D - 2 * M - M1));
	l += -1773.0 * FMath::Sin(FMath::DegreesToRadians(2 * D + M1 - 2 * F));
	l += -1595.0 * FMath::Sin(FMath::DegreesToRadians(2 * D + 2 * F));
	l += 1215.0 * E * FMath::Sin(FMath::DegreesToRadians(4 * D - M - M1));
	l += -1110.0 * FMath::Sin(FMath::DegreesToRadians(2 * M1 + 2 * F));
	l += -892.0 * FMath::Sin(FMath::DegreesToRadians(3 * D - M1));
	l += -810.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D + M + M1));
	l += 759.0 * E * FMath::Sin(FMath::DegreesToRadians(4 * D - M - 2 * M1));
	l += -713.0 * E * E * FMath::Sin(FMath::DegreesToRadians(2 * M - M1));
	l += -700.0 * E * E * FMath::Sin(FMath::DegreesToRadians(2 * D + 2 * M - M1));
	l += 691.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D + M - 2 * M1));
	l += 596.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D - M - 2 * F));
	l += 549.0 * FMath::Sin(FMath::DegreesToRadians(4 * D + M1));
	l += 537.0 * FMath::Sin(FMath::DegreesToRadians(4 * M1));
	l += 520.0 * E * FMath::Sin(FMath::DegreesToRadians(4 * D - M));
	l += -487.0 * FMath::Sin(FMath::DegreesToRadians(D - 2 * M1));
	l += -399.0 * FMath::Sin(FMath::DegreesToRadians(2 * D + M - 2 * F));
	l += -381.0 * FMath::Sin(FMath::DegreesToRadians(6 * D - M1));
	l += 351.0 * E * FMath::Sin(FMath::DegreesToRadians(6 * D - 2 * M1));
	l += -334.0 * FMath::Sin(FMath::DegreesToRadians(3 * M1 - 2 * F));
	l += -306.0 * FMath::Sin(FMath::DegreesToRadians(2 * D + 2 * F + M1));
	l += -300.0 * FMath::Sin(FMath::DegreesToRadians(M1 + F));
	return l; // 返回总计的周期项值（单位：角秒/10000）
}
// 月球纬度周期项求和（60 项三角级数展开）
// 计算月球视黄纬的周期项
double getSumOfPeriodicTermsOfLatitude(double D, double M, double M1, double F, double E, double A1, double A3, double L1)
{
	double b = 0.0;
	b += -2235.0 * FMath::Sin(FMath::DegreesToRadians(L1));
	b += 382.0 * FMath::Sin(FMath::DegreesToRadians(A3));
	b += 175.0 * FMath::Sin(FMath::DegreesToRadians(A1 - F));
	b += 175.0 * FMath::Sin(FMath::DegreesToRadians(A1 + F));
	b += 127.0 * FMath::Sin(FMath::DegreesToRadians(L1 - M1));
	b += -115.0 * FMath::Sin(FMath::DegreesToRadians(L1 + M1));
	b += 5128122.0 * FMath::Sin(FMath::DegreesToRadians(F));
	b += 280602.0 * FMath::Sin(FMath::DegreesToRadians(M1 + F));
	b += 277693.0 * FMath::Sin(FMath::DegreesToRadians(M1 - F));
	b += 173237.0 * FMath::Sin(FMath::DegreesToRadians(2 * D - F));
	b += 55413.0 * FMath::Sin(FMath::DegreesToRadians(2 * D - M1 + F));
	b += 46271.0 * FMath::Sin(FMath::DegreesToRadians(2 * D - M1 - F));
	b += 32573.0 * FMath::Sin(FMath::DegreesToRadians(2 * D + F));
	b += 17198.0 * FMath::Sin(FMath::DegreesToRadians(2 * M1 + F));
	b += 9266.0 * FMath::Sin(FMath::DegreesToRadians(2 * D + M1 - F));
	b += 8822.0 * FMath::Sin(FMath::DegreesToRadians(2 * M1 - F));
	b += 8216.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D - M - F));
	b += 4324.0 * FMath::Sin(FMath::DegreesToRadians(2 * D - 2 * M1 - F));
	b += 4200.0 * FMath::Sin(FMath::DegreesToRadians(2 * D + M1 + F));
	b += -3359.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D + M - F));
	b += 2463.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D - M - M1 + F));
	b += 2211.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D - M + F));
	b += 2065.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D - M - M1 - F));
	b += -1870.0 * FMath::Sin(FMath::DegreesToRadians(M1 - M - F));
	b += 1828.0 * FMath::Sin(FMath::DegreesToRadians(4 * D - M1 - F));
	b += -1794.0 * E * FMath::Sin(FMath::DegreesToRadians(M1 + F));
	b += -1749.0 * FMath::Sin(FMath::DegreesToRadians(3 * F));
	b += -1565.0 * E * FMath::Sin(FMath::DegreesToRadians(M - M1 - F));
	b += -1491.0 * FMath::Sin(FMath::DegreesToRadians(D + F));
	b += -1475.0 * E * FMath::Sin(FMath::DegreesToRadians(M + M1 + F));
	b += -1415.0 * E * FMath::Sin(FMath::DegreesToRadians(M + M1 - F));
	b += -1344.0 * E * FMath::Sin(FMath::DegreesToRadians(M - F));
	b += -1335.0 * FMath::Sin(FMath::DegreesToRadians(D - F));
	b += 1107.0 * FMath::Sin(FMath::DegreesToRadians(3 * M1 + F));
	b += 1021.0 * FMath::Sin(FMath::DegreesToRadians(4 * D - F));
	b += 833.0 * FMath::Sin(FMath::DegreesToRadians(4 * D - M1 + F));
	b += 777.0 * FMath::Sin(FMath::DegreesToRadians(M1 - 3 * F));
	b += 671.0 * FMath::Sin(FMath::DegreesToRadians(4 * D - 2 * M1 + F));
	b += 607.0 * FMath::Sin(FMath::DegreesToRadians(2 * D - 3 * F));
	b += 596.0 * FMath::Sin(FMath::DegreesToRadians(2 * D + 2 * M1 - F));
	b += 491.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D - M + M1 - F));
	b += -451.0 * FMath::Sin(FMath::DegreesToRadians(2 * D - 2 * M1 + F));
	b += 439.0 * FMath::Sin(FMath::DegreesToRadians(3 * M1 - F));
	b += 422.0 * FMath::Sin(FMath::DegreesToRadians(2 * D + 2 * M1 + F));
	b += 421.0 * FMath::Sin(FMath::DegreesToRadians(2 * D - 3 * M1 - F));
	b += -366.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D + M - M1 + F));
	b += -351.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D + M + F));
	b += 331.0 * FMath::Sin(FMath::DegreesToRadians(4 * D + F));
	b += 315.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D - M + M1 + F));
	b += 302.0 * E * E * FMath::Sin(FMath::DegreesToRadians(2 * D - 2 * M - F));
	b += -283.0 * FMath::Sin(FMath::DegreesToRadians(M1 + 3 * F));
	b += -229.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D + M + M1 - F));
	b += 223.0 * E * FMath::Sin(FMath::DegreesToRadians(D + M - F));
	b += 223.0 * E * FMath::Sin(FMath::DegreesToRadians(D + M + F));
	b += -220.0 * E * FMath::Sin(FMath::DegreesToRadians(M - 2 * M1 - F));
	b += -220.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D + M - M1 - F));
	b += -185.0 * FMath::Sin(FMath::DegreesToRadians(D + M1 + F));
	b += 181.0 * E * FMath::Sin(FMath::DegreesToRadians(2 * D - M - 2 * M1 - F));
	b += -177.0 * E * E * FMath::Sin(FMath::DegreesToRadians(M + 2 * M1 + F));
	b += 176.0 * FMath::Sin(FMath::DegreesToRadians(4 * D - 2 * M1 - F));
	b += 166.0 * E * E * FMath::Sin(FMath::DegreesToRadians(4 * D - M - M1 - F));
	b += -164.0 * E * FMath::Sin(FMath::DegreesToRadians(M1 + M - F));
	b += 132.0 * FMath::Sin(FMath::DegreesToRadians(4 * D + M1 - F));
	b += -119.0 * E * FMath::Sin(FMath::DegreesToRadians(D - M - F));
	b += 115.0 * E * FMath::Sin(FMath::DegreesToRadians(4 * D - M - F));
	b += 107.0 * E * E * FMath::Sin(FMath::DegreesToRadians(2 * D - 2 * M + F));
	return b; // 返回纬度周期项总值
}

// 月球距离周期项求和（计算地月距离的周期性变化）
double getSumOfPeriodicTermsOfDistance(double D, double M, double M1, double F, double E)
{
	double r = 0.0;
	r += -20905355.0 * FMath::Cos(FMath::DegreesToRadians(M1));
	r += -3699111.0 * FMath::Cos(FMath::DegreesToRadians(2 * D - M1));
	r += -2955968.0 * FMath::Cos(FMath::DegreesToRadians(2 * D));
	r += -569925.0 * FMath::Cos(FMath::DegreesToRadians(2 * M1));
	r += 48888.0 * E * FMath::Cos(FMath::DegreesToRadians(M));
	r += -3149.0 * FMath::Cos(FMath::DegreesToRadians(2 * F));
	r += 246158.0 * FMath::Cos(FMath::DegreesToRadians(2 * D - 2 * M1));
	r += -152138.0 * E * FMath::Cos(FMath::DegreesToRadians(2 * D - M - M1));
	r += -170733.0 * FMath::Cos(FMath::DegreesToRadians(2 * D + M1));
	r += -204586.0 * E * FMath::Cos(FMath::DegreesToRadians(2 * D - M));
	r += -129620.0 * E * FMath::Cos(FMath::DegreesToRadians(M - M1));
	r += 108743.0 * FMath::Cos(FMath::DegreesToRadians(D));
	r += 104755.0 * E * FMath::Cos(FMath::DegreesToRadians(M + M1));
	r += 10321.0 * FMath::Cos(FMath::DegreesToRadians(2 * D - 2 * F));
	r += 79661.0 * FMath::Cos(FMath::DegreesToRadians(M1 - 2 * F));
	r += -34782.0 * FMath::Cos(FMath::DegreesToRadians(4 * D - M1));
	r += -23210.0 * FMath::Cos(FMath::DegreesToRadians(3 * M1));
	r += -21636.0 * FMath::Cos(FMath::DegreesToRadians(4 * D - 2 * M1));
	r += 24208.0 * E * FMath::Cos(FMath::DegreesToRadians(2 * D + M - M1));
	r += 30824.0 * E * FMath::Cos(FMath::DegreesToRadians(2 * D + M));
	r += -8379.0 * FMath::Cos(FMath::DegreesToRadians(D - M1));
	r += -16675.0 * E * FMath::Cos(FMath::DegreesToRadians(D + M));
	r += -12831.0 * E * FMath::Cos(FMath::DegreesToRadians(2 * D - M + M1));
	r += -10445.0 * FMath::Cos(FMath::DegreesToRadians(2 * D + 2 * M1));
	r += -11650.0 * FMath::Cos(FMath::DegreesToRadians(4 * D));
	r += -14403.0 * FMath::Cos(FMath::DegreesToRadians(2 * D - 3 * M1));
	r += -7003.0 * E * FMath::Cos(FMath::DegreesToRadians(M - 2 * M1));
	r += 10056.0 * E * FMath::Cos(FMath::DegreesToRadians(2 * D - M - 2 * M1));
	r += 6322.0 * FMath::Cos(FMath::DegreesToRadians(2 * M1 + M - F));
	r += -9884.0 * E * FMath::Cos(FMath::DegreesToRadians(2 * D - M + F));
	r += 5751.0 * E * E * FMath::Cos(FMath::DegreesToRadians(2 * M));
	r += -4950.0 * E * E * FMath::Cos(FMath::DegreesToRadians(2 * D - 2 * M - M1));
	r += -4130.0 * FMath::Cos(FMath::DegreesToRadians(2 * D + M1 - 2 * F));
	r += -3958.0 * E * FMath::Cos(FMath::DegreesToRadians(4 * D - M - M1));
	r += -3258.0 * FMath::Cos(FMath::DegreesToRadians(3 * D - M1));
	r += 2616.0 * E * FMath::Cos(FMath::DegreesToRadians(2 * D + M + M1));
	r += -1897.0 * E * FMath::Cos(FMath::DegreesToRadians(4 * D - M - 2 * M1));
	r += -2117.0 * E * E * FMath::Cos(FMath::DegreesToRadians(2 * M - M1));
	r += 2354.0 * E * E * FMath::Cos(FMath::DegreesToRadians(2 * D + 2 * M - M1));
	r += -1423.0 * FMath::Cos(FMath::DegreesToRadians(4 * D + M1));
	r += -1117.0 * FMath::Cos(FMath::DegreesToRadians(4 * M1));
	r += -1571.0 * E * FMath::Cos(FMath::DegreesToRadians(4 * D - M));
	r += -1739.0 * FMath::Cos(FMath::DegreesToRadians(D - 2 * M1));
	r += -4421.0 * FMath::Cos(FMath::DegreesToRadians(2 * M1 - 2 * F));
	r += 1165.0 * E * E * FMath::Cos(FMath::DegreesToRadians(2 * M + M1));
	r += -1000.0 * FMath::Cos(FMath::DegreesToRadians(2 * D + 2 * F + M1));
	r += -875.0 * FMath::Cos(FMath::DegreesToRadians(D + M1));
	r += 300.0 * FMath::Cos(FMath::DegreesToRadians(2 * D - 2 * F + M1));
	r += 400.0 * FMath::Cos(FMath::DegreesToRadians(2 * D - 3 * F - M1));
	r += 250.0 * FMath::Cos(FMath::DegreesToRadians(3 * D - F));
	r += 225.0 * FMath::Cos(FMath::DegreesToRadians(6 * D - 2 * F));
	return r; // 返回距离周期项总值
}
// ============================================================
// 辅助计算函数
// ============================================================

// 计算格林尼治视恒星时 (GAST)
double getGAST(double GST0, double UT)
{
	double GAST = fmod(GST0 + 1.00273790935 * UT, 360.0); // 世界时 UT 到恒星时的转换
	if (GAST < 0.0) // 确保结果在 [0, 360) 范围内
	{
		GAST += 360.0;
	}
	return GAST;
}

// 计算 GST0（世界时 0h 的格林尼治恒星时）
double getGST0(double Seconds, double Minutes, double Hours, double Day, double Month, double Year)
{
	double JD = getJD(Day, Month, Year) - 2451545.0; // 儒略世纪数（J2000.0 为基准）
	double T = JD / 36525.0; // 儒略世纪数
	double GST0 = 100.46061837 + 36000.770053608 * T + 0.000387933 * T * T - 0.000002583 * T * T * T; // IAU 2000 公式
	GST0 = fmod(GST0, 360.0); // 归一化到 [0, 360)
	if (GST0 < 0.0)
	{
		GST0 += 360.0;
	}
	return GST0;
}

// 计算恒星时角
double getHourAngle(double GST0, double UT, double RightAscension, double Longitude)
{
	double GAST = getGAST(GST0, UT); // 格林尼治视恒星时
	double LHA = GAST + Longitude - RightAscension; // 地方时角 = GAST + 经度 - 赤经
	LHA = fmod(LHA, 360.0); // 归一化
	if (LHA < 0.0)
	{
		LHA += 360.0;
	}
	return LHA;
}

// 根据赤纬和时角计算高度角
double getElevation(double Declination, double HourAngle, double Latitude)
{
	double Elevation = FMath::RadiansToDegrees(FMath::Asin(
		FMath::Sin(FMath::DegreesToRadians(Declination)) * FMath::Sin(FMath::DegreesToRadians(Latitude)) +
		FMath::Cos(FMath::DegreesToRadians(Declination)) * FMath::Cos(FMath::DegreesToRadians(HourAngle)) * FMath::Cos(FMath::DegreesToRadians(Latitude))));
	return Elevation;
}

// 根据赤纬和时角计算方位角
double getAzimuth(double Declination, double HourAngle, double Latitude)
{
	double Azimuth = FMath::RadiansToDegrees(FMath::Atan2(
		FMath::Sin(FMath::DegreesToRadians(HourAngle)),
		FMath::Cos(FMath::DegreesToRadians(HourAngle)) * FMath::Sin(FMath::DegreesToRadians(Latitude)) -
		FMath::Tan(FMath::DegreesToRadians(Declination)) * FMath::Cos(FMath::DegreesToRadians(Latitude))));
	return Azimuth + 180.0; // 转换到天文学方位角（0=南 → 0=北）
}

// 计算天体在视场中的旋转角度（用于 skybox 中星体贴图的旋转校正）
double getRotationAngle(double Declination, double RightAscension, double GST0, double UT, double Longitude, double Latitude)
{
	double HA = getHourAngle(GST0, UT, RightAscension, Longitude); // 地方时角
	double y = FMath::Sin(FMath::DegreesToRadians(HA));
	double x = FMath::Tan(FMath::DegreesToRadians(Declination)) * FMath::Cos(FMath::DegreesToRadians(Latitude)) -
		FMath::Sin(FMath::DegreesToRadians(Latitude)) * FMath::Cos(FMath::DegreesToRadians(HA));
	double RotationAngle = FMath::RadiansToDegrees(FMath::Atan2(y, x));
	return RotationAngle;
}
// ============================================================
// 月球位置计算 (Sky Position Plugin)
// 根据地理坐标和日期时间，计算月球的高度角、方位角、距离、月相照明率等
// ============================================================
void UCWeatherBPFLibrary::GetMoonPositionSPP(float Latitude, float Longitude, float TimeZone, float NorthOffset, bool bIsDaylightSavingTime, int32 Year, int32 Month, int32 Day, int32 Hours, int32 Minutes, int32 Seconds, FMMoonData& MoonData)
{
	// 计算儒略日 (JDE)
	double JDE = getJD(Day, Month, Year); // 将公历日期转换为儒略日
	double JD = JDE - 2451545.0; // 转换为以 J2000.0 为起点的儒略世纪数
	double T = JD / 36525.0; // 儒略世纪数（约 100 年）

	// 将本地时间转换为 UTC 世界时
	double TimeOffset = (bIsDaylightSavingTime) ? TimeZone + 1.0 : TimeZone; // 夏令时则 +1 时区偏移
	double DHours = double(Hours); // 时
	double DMinutes = double(Minutes); // 分
	double DSeconds = double(Seconds); // 秒
	FDateTime LocalDate(Year, Month, Day, DHours, DMinutes, DSeconds); // 本地日期时间
	// 计算日小数部分
	int32 DDay = int32((fmod(TimeOffset, 24.0) - DHours) / 24.0); // 时区偏移导致的日期修正（天）
	DHours = DHours - int32(fmod(TimeOffset, 24.0) - DHours) * 1.0; // 调整后的时
	DMinutes = int32(((fmod(TimeOffset, 24.0) - DHours) * 60.0 - DMinutes)); // 调整后的分
	DSeconds = int32((((fmod(TimeOffset, 24.0) - DHours) * 60.0 - DMinutes) * 60.0)); // 调整后的秒
	int32 DMilliseconds = int32((((fmod(TimeOffset, 24.0) - DHours) * 60.0 - DMinutes) * 60.0 - DSeconds) * 1000.0); // 调整后的毫秒
	FTimespan DeltaDate(DDay, DHours, DMinutes, DSeconds, DMilliseconds); // 时区偏移时间跨度
	FDateTime UniversalDate = LocalDate - DeltaDate; // 本地时间减偏移 = UTC 时间
	int32 UYear = UniversalDate.GetYear(); // UTC 年
	int32 UMonth = UniversalDate.GetMonth(); // UTC 月
	int32 UDay = UniversalDate.GetDay(); // UTC 日
	int32 UHours = UniversalDate.GetHour(); // UTC 时
	int32 UMinutes = UniversalDate.GetMinute(); // UTC 分
	int32 USeconds = UniversalDate.GetSecond(); // UTC 秒
	double UT = double(UHours) + double(UMinutes) / 60.0 + double(USeconds) / 3600.0; // UTC 时间（小数小时）

	// --- 月球平轨道参数计算（基于 VSOP87 简化模型）---
	double L1 = 218.3164477 + 481267.88123421 * T - 0.0015786 * T * T + T * T * T / 538841.0 - T * T * T * T / 65194000.0; // 月球平经度
	L1 = fmod(L1, 360.0); if (L1 < 0.0) L1 += 360.0;
	double D = 297.8501921 + 445267.1114034 * T - 0.0018819 * T * T + T * T * T / 545868.0 - T * T * T * T / 113065000.0; // 月球平距角
	D = fmod(D, 360.0); if (D < 0.0) D += 360.0;
	double M = 357.5291092 + 35999.0502909 * T - 0.0001536 * T * T + T * T * T / 24490000.0; // 太阳平近点角
	M = fmod(M, 360.0); if (M < 0.0) M += 360.0;
	double M1 = 134.9633964 + 477198.8675055 * T + 0.0087414 * T * T + T * T * T / 69699.0 - T * T * T * T / 14712000.0; // 月球平近点角
	M1 = fmod(M1, 360.0); if (M1 < 0.0) M1 += 360.0;
	double F = 93.2720950 + 483202.0175233 * T - 0.0036539 * T * T - T * T * T / 3526000.0 + T * T * T * T / 863310000.0; // 月球纬度参数（升交点距角）
	F = fmod(F, 360.0); if (F < 0.0) F += 360.0;
	double A1 = 119.75 + 131.849 * T; // 金星摄动项 1
	A1 = fmod(A1, 360.0); if (A1 < 0.0) A1 += 360.0;
	double A2 = 53.09 + 479264.290 * T; // 金星摄动项 2
	A2 = fmod(A2, 360.0); if (A2 < 0.0) A2 += 360.0;
	double A3 = 313.45 + 481266.484 * T; // 金星摄动项 3
	A3 = fmod(A3, 360.0); if (A3 < 0.0) A3 += 360.0;
	double E = 1.0 - 0.002516 * T - 0.0000074 * T * T; // 轨道偏心率修正

	// --- 计算月球视经度、视纬度、视距离 ---
	double SumL = getSumOfPeriodicTermsOfLongitude(D, M, M1, F, E, A1, A2, L1); // 经度周期项求和
	double SumB = getSumOfPeriodicTermsOfLatitude(D, M, M1, F, E, A1, A3, L1); // 纬度周期项求和
	double SumR = getSumOfPeriodicTermsOfDistance(D, M, M1, F, E); // 距离周期项求和

	double ApparentMoonLongitude = L1 + SumL / 1000000.0; // 月球视经度 = 平经度 + 周期项修正
	double ApparentMoonLatitude = SumB / 1000000.0; // 月球视纬度
	double ApparentMoonDistance = 385000.56 + SumR / 1000.0; // 月球视距离 (km)

	// --- 章动和真黄赤交角 ---
	double Omega = 125.04 - 1934.136 * T; // 月球升交点平经度
	Omega = fmod(Omega, 360.0); if (Omega < 0.0) Omega += 360.0;
	double L2 = L1 + SumL / 1000000.0; // 月球视经度（用于章动）
	double DeltaPsi = -17.20 * FMath::Sin(FMath::DegreesToRadians(Omega)) - 1.32 * FMath::Sin(FMath::DegreesToRadians(2.0 * L2)) - 0.23 * FMath::Sin(FMath::DegreesToRadians(2.0 * D)) + 0.21 * FMath::Sin(FMath::DegreesToRadians(2.0 * Omega)); // 黄经章动（角秒）
	double DeltaEpsilon = 9.20 * FMath::Cos(FMath::DegreesToRadians(Omega)) + 0.57 * FMath::Cos(FMath::DegreesToRadians(2.0 * L2)) + 0.10 * FMath::Cos(FMath::DegreesToRadians(2.0 * D)) - 0.09 * FMath::Cos(FMath::DegreesToRadians(2.0 * Omega)); // 交角章动（角秒）
	double Epsilon0 = 23.0 + 26.0 / 60.0 + 21.448 / 3600.0 - 46.8150 / 3600.0 * T - 0.00059 / 3600.0 * T * T + 0.001813 / 3600.0 * T * T * T; // 平黄赤交角
	double TrueEpsilon = Epsilon0 + DeltaEpsilon / 3600.0; // 真黄赤交角 = 平交角 + 章动修正

	// --- 视差校正 ---
	double Parallax = FMath::RadiansToDegrees(FMath::Asin(6378.14 / ApparentMoonDistance)); // 地平视差 = arcsin(地球赤道半径 / 月地距离)

	// --- 赤经赤纬计算 ---
	double YValue = (FMath::Sin(FMath::DegreesToRadians(ApparentMoonLongitude)) * FMath::Cos(FMath::DegreesToRadians(TrueEpsilon)) - FMath::Tan(FMath::DegreesToRadians(ApparentMoonLatitude)) * FMath::Sin(FMath::DegreesToRadians(TrueEpsilon))); // 赤经辅助变量
	double XValue = FMath::Cos(FMath::DegreesToRadians(ApparentMoonLongitude)); // 赤经辅助变量
	double RightAscension = fmod(FMath::RadiansToDegrees(FMath::Atan2(YValue, XValue)), 360.0); // 赤经
	if (RightAscension < 0.0) RightAscension += 360.0;
	double Declination = FMath::RadiansToDegrees(FMath::Asin(FMath::Sin(FMath::DegreesToRadians(ApparentMoonLatitude)) * FMath::Cos(FMath::DegreesToRadians(TrueEpsilon)) + FMath::Cos(FMath::DegreesToRadians(ApparentMoonLatitude)) * FMath::Sin(FMath::DegreesToRadians(TrueEpsilon)) * FMath::Sin(FMath::DegreesToRadians(ApparentMoonLongitude)))); // 赤纬

	// --- GST0 和视位置 ---
	double GST0 = getGST0(USeconds, UMinutes, UHours, UDay, UMonth, UYear); // 计算 GST0
	double HourAngle = getHourAngle(GST0, UT, RightAscension, Longitude); // 地方时角

	// --- 高度角 ---
	double Elevation = getElevation(Declination, HourAngle, Latitude); // 未修正地平视差的高度角
	double ElevationCorrected = Elevation + Parallax * FMath::Cos(FMath::DegreesToRadians(Elevation)); // 修正大气折射和视差后的高度角

	// --- 方位角 ---
	double Azimuth = getAzimuth(Declination, HourAngle, Latitude); // 天文学方位角

	// --- 自转角度 ---
	double RotationAngle = getRotationAngle(Declination, RightAscension, GST0, UT, Longitude, Latitude); // 月球在视场中的旋转角度

	// --- 天平动 ---
	double W = L1 - Omega; // 月球轨道经度参数
	W = fmod(W, 360.0); if (W < 0.0) W += 360.0;
	const double I = 1.54242;
	double Libr_Lat = -1.0 * UKismetMathLibrary::SelectFloat(1.0, -1.0, FMath::Sin(FMath::DegreesToRadians(F - Omega)) > 0.0) * FMath::Acos(FMath::Cos(FMath::DegreesToRadians(F - Omega))); // 纬度天平动
	double Libr_Long = -1.0 * (FMath::RadiansToDegrees(FMath::Atan2(FMath::Sin(FMath::DegreesToRadians(W)), FMath::Cos(FMath::DegreesToRadians(W)) - FMath::Cos(FMath::DegreesToRadians(I)) * FMath::Sin(FMath::DegreesToRadians(W))))); // 经度天平动


// ...
// --- 月球截面照明率 ---
	double DiskIllumination = (1.0 + UKismetMathLibrary::Cos(FMath::DegreesToRadians(L1 - L2))) / 2.0; // 月相照明比例（0=新月，1=满月）

	// --- 位置角 ---
	double PositionAngle = FMath::RadiansToDegrees(FMath::Atan2(FMath::Sin(FMath::DegreesToRadians(L1 - Omega)), FMath::Cos(FMath::DegreesToRadians(L1 - Omega)) * FMath::Sin(FMath::DegreesToRadians(TrueEpsilon)))); // 亮部中心位置角

	// --- 输出到结构体 ---
	MoonData.Elevation = ElevationCorrected; // 高度角（度）
	MoonData.Azimuth = Azimuth + NorthOffset; // 方位角 + 指北偏移
	MoonData.RotationAngle = RotationAngle; // 自转角度
	MoonData.DistanceToTheMoon = ApparentMoonDistance; // 地月距离（km）
	MoonData.LibrationInLatitude = Libr_Lat; // 纬度天平动
	MoonData.LibrationInLongitude = Libr_Long; // 经度天平动
	MoonData.PositionAngle = PositionAngle; // 位置角
	MoonData.DiskIllumination = DiskIllumination; // 月面照亮百分比
	MoonData.Declination = Declination; // 赤纬
	MoonData.RightAscension = RightAscension; // 赤经（度）
}

// ============================================================
// 太阳位置计算 (Sky Position Plugin)
// 使用 UE5 内置的 SunPosition 库简化版本
// ============================================================
void UCWeatherBPFLibrary::GetSunPositionSPP(float Latitude, float Longitude, float TimeZone, float NorthOffset, bool bIsDaylightSavingTime, int32 Year, int32 Month, int32 Day, int32 Hours, int32 Minutes, int32 Seconds, FMSunData& SunData)
{
	FSunPositionData tSunPositionData; // UE5 内置太阳位置数据结构
	// 注意：USunPositionFunctionLibrary 原始注释标记为"此方法已废弃" (true)，但当前版本仍可使用
	USunPositionFunctionLibrary::GetSunPosition(Latitude, Longitude, TimeZone, bIsDaylightSavingTime, Year, Month, Day, Hours, Minutes, Seconds, tSunPositionData);
	SunData.Elevation = tSunPositionData.CorrectedElevation; // 修正后的太阳高度角
	SunData.Azimuth = tSunPositionData.Azimuth - 90 + NorthOffset; // 太阳方位角（-90 将正南转为正东基准）
	SunData.Declination = 0; // 未计算赤纬（使用引擎内置库时此值不可用）
	SunData.RightAscension = 0; // 未计算赤经
	SunData.Distance = 0; // 未计算日地距离
}

// ============================================================
// 恒星/星空位置计算 (Sky Position Plugin)
// 基于格林尼治恒星时(GST0)与地方时角计算星空旋转
// ============================================================
void UCWeatherBPFLibrary::GetStarsPositionSPP(float Latitude, float Longitude, float TimeZone, float NorthOffset, bool bIsDaylightSavingTime, int32 Year, int32 Month, int32 Day, int32 Hours, int32 Minutes, int32 Seconds, FRotator& StarSphereRotator)
{
	// 本地时间转 UTC
	double TimeOffset = (bIsDaylightSavingTime) ? TimeZone + 1.0 : TimeZone; // 夏令时调整
	double DHours = double(Hours);
	double DMinutes = double(Minutes);
	double DSeconds = double(Seconds);
	FDateTime LocalDate(Year, Month, Day, DHours, DMinutes, DSeconds); // 本地时间
	int32 DDay = int32((fmod(TimeOffset, 24.0) - DHours) / 24.0); // 时区偏移日修正
	DHours = DHours - int32(fmod(TimeOffset, 24.0) - DHours); // 调整后小时
	DMinutes = int32(((fmod(TimeOffset, 24.0) - DHours) * 60.0 - DMinutes)); // 调整后分钟
	DSeconds = int32((((fmod(TimeOffset, 24.0) - DHours) * 60.0 - DMinutes) * 60.0)); // 调整后秒
	int32 DMilliseconds = int32((((fmod(TimeOffset, 24.0) - DHours) * 60.0 - DMinutes) * 60.0 - DSeconds) * 1000.0); // 调整后毫秒
	FTimespan DeltaDate(DDay, DHours, DMinutes, DSeconds, DMilliseconds); // 时区偏移
	FDateTime UniversalDate = LocalDate - DeltaDate; // UTC 时间
	int32 UYear = UniversalDate.GetYear();
	int32 UMonth = UniversalDate.GetMonth();
	int32 UDay = UniversalDate.GetDay();
	int32 UHours = UniversalDate.GetHour();
	int32 UMinutes = UniversalDate.GetMinute();
	int32 USeconds = UniversalDate.GetSecond();
	double UT = double(UHours) + double(UMinutes) / 60.0 + double(USeconds) / 3600.0; // UTC 时间（小数小时）

	// 计算 GST0（UTC 0h 恒星时）
	double GST0 = getGST0(USeconds, UMinutes, UHours, UDay, UMonth, UYear);

	// 春分点作为参考位置（赤经=0, 赤纬=0），计算其方位和高度以确定星空旋转
	double Point1Declination = 0.0; // 春分点赤纬 = 0
	double Point1RightAscension = 0.0; // 春分点赤经 = 0
	double Point1HourAngle = getHourAngle(GST0, UT, Point1RightAscension, Longitude); // 春分点地方时角
	double Point1Elevation = getElevation(Point1Declination, Point1HourAngle, Latitude); // 春分点高度角
	double Point1Azimuth = getAzimuth(Point1Declination, Point1HourAngle, Latitude); // 春分点方位角

	// 计算旋转角度
	double Roll = getRotationAngle(Point1Declination, Point1RightAscension, GST0, UT, Longitude, Latitude); // 星空球 Roll 旋转
	double Pitch = Point1Elevation; // Pitch = 高度角
	double Yaw = Point1Azimuth + NorthOffset; // Yaw = 方位角 + 指北偏移

	// 输出星空球的旋转
	StarSphereRotator = FRotator(Pitch, Yaw, Roll);
}

// ============================================================
// 月面纹理旋转计算
// 根据天平动和位置角计算月面贴图的旋转
// ============================================================
void UCWeatherBPFLibrary::CalculateMoonRotationSPP(float LibrationInLatitude, float LibrationInLongitude, float PositionAngle, float RotationAngle, FRotator& MoonRotator)
{
	FVector FirstVector = FVector(-1.0, 0.0, 0.0); // 参考向量（-X 方向）
	FVector SecondVector = UKismetMathLibrary::Conv_RotatorToVector(FRotator(LibrationInLatitude, 180.0 - LibrationInLongitude, 0.0)); // 天平动方向向量
	float k = 1.0 / sqrt(1.0 - pow(SecondVector.Y, 2)); // 正交化因子
	float Pitch = UKismetMathLibrary::SelectInt(-1, 1, LibrationInLatitude < 0.0) * FMath::RadiansToDegrees(FMath::Acos(UKismetMathLibrary::Dot_VectorVector(k * (SecondVector + FVector(0.0, -1.0 * SecondVector.Y, 0.0)), FirstVector))); // 根据纬度天平动计算 Pitch
	float Yaw = UKismetMathLibrary::SelectInt(-1, 1, LibrationInLongitude < 0.0) * FMath::RadiansToDegrees(FMath::Acos(UKismetMathLibrary::Dot_VectorVector(k * FVector(SecondVector.X, 0.0, SecondVector.Z), SecondVector))); // 根据经度天平动计算 Yaw
	float Roll = -1.0 * PositionAngle + RotationAngle; // Roll = -位置角 + 自转角度
	MoonRotator = FRotator(Pitch, Yaw, Roll); // 输出月面旋转
}


// --- 角度差计算 ---
// 计算向量与上方向的夹角，带方向符号（叉积判断左右）
float UCWeatherBPFLibrary::CalcDeltaAngle(const FVector& InAngle, const FVector& NormalUp, const FVector& NormalForward)
{
	float OutAngle = 90.0f - UKismetMathLibrary::DegAcos(FVector::DotProduct(InAngle, NormalUp)); // 与上方向的夹角
	return (FVector::DotProduct(FVector::CrossProduct(InAngle, NormalUp), NormalForward) < 0) ? 180.0 - OutAngle : OutAngle; // 叉积判断方向，左侧为负则翻转
}

// --- 太阳时旋转计算 ---
// 根据太阳时和地理坐标计算太阳在场景中的旋转（Pitch=高度角，Yaw=方位角-90）
FRotator UCWeatherBPFLibrary::CalcSolarTimeRot(float SolarTime, float Latitude, float Longitude, float TimeZone, int32 Year, int32 Month, int32 Day)
{
	int32 tHours, tMinutes, tSeconds; // 时/分/秒
	FSunPositionData tSunPositionData; // 太阳位置数据
	PGetHMSFromSolarTime(SolarTime, tHours, tMinutes, tSeconds); // 将太阳时转换为时分秒
	USunPositionFunctionLibrary::GetSunPosition(Latitude, Longitude, TimeZone, false, Year, Month, Day, tHours, tMinutes, tSeconds, tSunPositionData); // 获取太阳位置
	return FRotator(tSunPositionData.CorrectedElevation, tSunPositionData.Azimuth - 90, 0); // 构建旋转（Pitch=修正高度角，Yaw=方位角-90 转为以东为基准）
}
// --- Cesium 坐标变换 ---
// 将 ENU (East-North-Up) 旋转转换为 UE 世界空间旋转
FRotator UCWeatherBPFLibrary::TransformRotationENUtoUe(ACesiumGeoreference* GeoReference, FVector UeLoc, FRotator EnuRot)
{
	EnuRot.Yaw = EnuRot.Yaw - 90; // ENU 坐标 Yaw=0 为正东，UE 坐标 Yaw=0 为正北，减去 90 度对齐
	FRotator ResultRot; // 结果旋转
	if (IsValid(GeoReference)) // 确保地理参考 Actor 有效
	{
		// 使用 Cesium 的 ComputeEastSouthUpToUnrealTransformation 计算旋转变换矩阵
		FQuat EnuToUeQuat = GeoReference->ComputeEastSouthUpToUnrealTransformation(UeLoc).ToQuat(); // 获取 ENU→UE 变换的四元数表示
		// 旧版 API: GeoReference->InaccurateComputeEastSouthUpToUnreal(UeLoc).ToQuat();
		FQuat EnuRotQuat = EnuRot.Quaternion(); // 将输入 ENU 旋转转为四元数
		ResultRot = (EnuToUeQuat * EnuRotQuat).Rotator(); // 左乘变换四元数后转回 Rotator
	}
	return ResultRot; // 返回 UE 空间的旋转
}

// 将经纬高字符串转换为 UE 世界坐标
FVector UCWeatherBPFLibrary::TransformLonLatHeightToUe(ACesiumGeoreference* GeoReference, FString Lon, FString Lat, FString Height)
{
	glm::dvec3 tVec; // GLM 双精度 3D 向量
	if (IsValid(GeoReference)) // 确保地理参考有效
	{
		tVec.x = FCString::Atod(*Lon); // 字符串转 double → 经度
		tVec.y = FCString::Atod(*Lat); // 纬度
		tVec.z = FCString::Atod(*Height); // 高度（米）

		FVector FVec(tVec.x, tVec.y, tVec.z); // glm::dvec3 → FVector（可能丢失精度）
		return GeoReference->TransformLongitudeLatitudeHeightPositionToUnreal(FVec); // Cesium 坐标转换至 UE 世界位置
	}
	return FVector(); // 返回零向量表示失败
}

// 获取 UE 绝对世界到 ECEF (地心固连坐标系) 的 4x4 变换矩阵
FMatrix UCWeatherBPFLibrary::GetAbsUeToEcefMatrix(ACesiumGeoreference* InGeoRef)
{
	if (InGeoRef->IsValidLowLevel()) // 验证 UObject 有效性
	{
		const GeoTransforms& GeoTrans = InGeoRef->GetGeoTransforms(); // 获取 Cesium 地理变换对象引用
		glm::dmat4 AbsUeToEcef = GeoTrans.GetAbsoluteUnrealWorldToEllipsoidCenteredTransform(); // 获取 4x4 双精度变换矩阵
		FMatrix Result; // UE 的 4x4 矩阵
		for (int32 i = 0; i < 4; ++i) { // 行
			for (int32 j = 0; j < 4; ++j) { // 列
				Result.M[i][j] = static_cast<float>(AbsUeToEcef[j][i]); // GLM 列主序 → UE 行主序，注意行列转置 (j,i 而不是 i,j)
			}
		}
		return Result; // 返回 float 精度的 ECEF 变换矩阵
	}
	return FMatrix::Identity; // 返回单位矩阵作为兜底
}