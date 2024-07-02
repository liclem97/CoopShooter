#pragma once

#define TRACE_LENGTH 80000.f

#define CUSTOM_DEPTH_PURPLE 250
#define CUSTOM_DEPTH_BLUE 251
#define CUSTOM_DEPTH_TAN 252

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	EWT_SubmachineGun UMETA(DisplayName = "Submachine Gun"),
	EWT_Shotgun UMETA(DisplayName = "ShotGun"),
	EWT_SniperRifle UMETA(DisplayName = "SniperRifle"),
	EWT_GrenadeLauncher UMETA(DisplayName = "Grenade Launcher"),

	EWT_MAX UMETA(DisplayName = "DefaultMAX")
};

namespace WeaponTypeString
{
	const FString AssaultRifle(TEXT("Assault Rifle"));
	const FString RocketLauncher(TEXT("Rocket Launcher"));
	const FString Pistol(TEXT("Pistol"));
	const FString SubmachineGun(TEXT("Submachine Gun"));
	const FString Shotgun(TEXT("ShotGun"));
	const FString SniperRifle(TEXT("Sniper Rifle"));
	const FString GrenadeLauncher(TEXT("Grenade Launcher"));
	const FString EmptyWeapon(TEXT(""));
}