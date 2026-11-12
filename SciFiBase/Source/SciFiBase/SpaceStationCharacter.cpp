#include "SpaceStationCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputMappingContext.h"

ASpaceStationCharacter::ASpaceStationCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Инициализация статов
	MaxHealth = 100.0f;
	Health = MaxHealth;
	MaxStamina = 100.0f;
	Stamina = MaxStamina;
	bIsDead = false;

	// Инициализация параметров движения
	WalkSpeed = 600.0f;
	SprintSpeed = 1000.0f;
	StaminaDrainRate = 25.0f;
	StaminaRegenRate = 20.0f;
	bIsSprinting = false;

	// Установка базовой скорости ходьбы
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

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

	GetCharacterMovement()->bOrientRotationToMovement = false; // Отключаем поворот лицом по ходу движения
	GetCharacterMovement()->bUseControllerDesiredRotation = true; // Включаем плавний поворот за камерой!
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // Скорость поворота (чем больше, тем быстрее крутится)
}

void ASpaceStationCharacter::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	Stamina = MaxStamina;

	// Добавляем Input Mapping Context (IMC_Default) в подсистему ввода игрока
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void ASpaceStationCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateStamina(DeltaTime);
}

void ASpaceStationCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Привязка движения (WASD)
		if (IA_Move)
		{
			EnhancedInputComp->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ASpaceStationCharacter::Move);
		}

		// Привязка вращения камеры (Мышь)
		if (IA_Look)
		{
			EnhancedInputComp->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ASpaceStationCharacter::Look);
		}

		// Привязка спринта (Shift)
		if (IA_Sprint)
		{
			EnhancedInputComp->BindAction(IA_Sprint, ETriggerEvent::Started, this, &ASpaceStationCharacter::StartSprint);
			EnhancedInputComp->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &ASpaceStationCharacter::StopSprint);
		}
	}
}

void ASpaceStationCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ASpaceStationCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
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
	StopSprint();
	GetCharacterMovement()->DisableMovement();
	UE_LOG(LogTemp, Warning, TEXT("Character has died!"));
}

void ASpaceStationCharacter::StartSprint()
{
	if (bIsDead || Stamina <= 0.0f) return;

	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void ASpaceStationCharacter::StopSprint()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ASpaceStationCharacter::UpdateStamina(float DeltaTime)
{
	bool bIsMoving = GetVelocity().SizeSquared2D() > 10.0f;

	if (bIsSprinting && bIsMoving)
	{
		Stamina = FMath::Clamp(Stamina - (StaminaDrainRate * DeltaTime), 0.0f, MaxStamina);

		if (Stamina <= 0.0f)
		{
			StopSprint();
		}
	}
	else
	{
		Stamina = FMath::Clamp(Stamina + (StaminaRegenRate * DeltaTime), 0.0f, MaxStamina);
	}
}