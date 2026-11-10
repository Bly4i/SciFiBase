#include "SpaceStationCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ASpaceStationCharacter::ASpaceStationCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Инициализация статов
	MaxHealth = 100.0f;
	Health = MaxHealth;
	MaxStamina = 100.0f;
	Stamina = MaxStamina;
	bIsDead = false;

	// Настройка SpringArm (Плечо камеры)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->bUsePawnControlRotation = true; // Камера вращается вместе с контроллером

	// Настройка Камеры
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // Камера не вращается относительно плеча

	// Настройки вращения персонажа
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
}

void ASpaceStationCharacter::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	Stamina = MaxStamina;
}

void ASpaceStationCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASpaceStationCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ASpaceStationCharacter::TakeCustomDamage_Implementation(float DamageAmount, AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.0f) return;

	Health = FMath::Clamp(Health - DamageAmount, 0.0f, MaxHealth);

	if (Health <= 0.0f)
	{
		Die();
	}
}

void ASpaceStationCharacter::Die()
{
	if (bIsDead) return;

	bIsDead = true;
	// Отключаем ввод и коллизию движений при смерти
	GetCharacterMovement()->DisableMovement();
	UE_LOG(LogTemp, Warning, TEXT("Character has died!"));
}