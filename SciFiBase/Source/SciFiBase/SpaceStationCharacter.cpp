#include "SpaceStationCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Weapon.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "DrawDebugHelpers.h"

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
	AimWalkSpeed = 300.0f;
	StaminaDrainRate = 25.0f;
	StaminaRegenRate = 20.0f;
	bIsSprinting = false;
	bIsAiming = false;

	// Установка базовой скорости ходьбы
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// Настройка SpringArm (Плечо камеры)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 180.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 65.0f, 30.0f);
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
	GetCharacterMovement()->bUseControllerDesiredRotation = true; // Включаем плавный поворот за камерой
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // Скорость поворота
}

void ASpaceStationCharacter::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	Stamina = MaxStamina;
	UpdateMovementSpeed();

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

	// Спавн и крепление оружия к сокету правой руки
	if (UWorld* World = GetWorld())
	{
		if (WeaponClass && GetMesh())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = this;

			EquippedWeapon = World->SpawnActor<AWeapon>(WeaponClass, FTransform::Identity, SpawnParams);
			if (EquippedWeapon)
			{
				EquippedWeapon->AttachToComponent(
					GetMesh(),
					FAttachmentTransformRules::SnapToTargetNotIncludingScale,
					WeaponSocketName
				);
			}
		}

		// Добавление прицела на экран локальному игроку
		if (IsLocallyControlled() && CrosshairWidgetClass)
		{
			if (UUserWidget* Crosshair = CreateWidget<UUserWidget>(World, CrosshairWidgetClass))
			{
				Crosshair->AddToViewport();
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

		// Привязка прицеливания (ПКМ по зажатию)
		if (IA_Aim)
		{
			EnhancedInputComp->BindAction(IA_Aim, ETriggerEvent::Started, this, &ASpaceStationCharacter::StartAim);
			EnhancedInputComp->BindAction(IA_Aim, ETriggerEvent::Completed, this, &ASpaceStationCharacter::StopAim);
		}

		// Привязка стрельбы (ЛКМ)
		if (IA_Fire)
		{
			EnhancedInputComp->BindAction(IA_Fire, ETriggerEvent::Started, this, &ASpaceStationCharacter::Fire);
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

void ASpaceStationCharacter::Fire()
{
	if (bIsDead || !EquippedWeapon)
	{
		return;
	}

	// Проигрываем монтаж выстрела
	if (FireMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(FireMontage);
	}

	// Базовый трейсер (лайнтрейс) для визуализации выстрела
	if (UWorld* World = GetWorld())
	{
		FVector TraceStart;
		FVector TraceEnd;
		const float TraceRange = 5000.0f;

		if (FollowCamera)
		{
			TraceStart = FollowCamera->GetComponentLocation();
			TraceEnd = TraceStart + (FollowCamera->GetForwardVector() * TraceRange);
		}
		else
		{
			FVector EyeLocation;
			FRotator EyeRotation;
			GetActorEyesViewPoint(EyeLocation, EyeRotation);
			TraceStart = EyeLocation;
			TraceEnd = TraceStart + (EyeRotation.Vector() * TraceRange);
		}

		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(EquippedWeapon);

		const bool bHit = World->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			ECC_Visibility,
			QueryParams
		);

		const FVector EndPoint = bHit ? HitResult.ImpactPoint : TraceEnd;

		// Рисуем линию трейсера на 1.5 секунды
		DrawDebugLine(
			World,
			TraceStart,
			EndPoint,
			bHit ? FColor::Red : FColor::Yellow,
			false,
			1.5f,
			0,
			2.0f
		);

		if (bHit)
		{
			DrawDebugPoint(World, HitResult.ImpactPoint, 8.0f, FColor::Red, false, 1.5f);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Fire pressed!"));
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
	bIsAiming = false;
	StopSprint();
	GetCharacterMovement()->DisableMovement();
	UE_LOG(LogTemp, Warning, TEXT("Character has died!"));
}

void ASpaceStationCharacter::StartSprint()
{
	if (bIsDead || bIsAiming || Stamina <= 0.0f) return;

	bIsSprinting = true;
	UpdateMovementSpeed();
}

void ASpaceStationCharacter::StopSprint()
{
	bIsSprinting = false;
	UpdateMovementSpeed();
}

void ASpaceStationCharacter::StartAim()
{
	if (bIsDead) return;

	bIsAiming = true;
	bIsSprinting = false;
	UpdateMovementSpeed();
}

void ASpaceStationCharacter::StopAim()
{
	bIsAiming = false;
	UpdateMovementSpeed();
}

void ASpaceStationCharacter::UpdateMovementSpeed()
{
	if (!GetCharacterMovement()) return;

	if (bIsAiming)
	{
		GetCharacterMovement()->MaxWalkSpeed = AimWalkSpeed;
	}
	else if (bIsSprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

void ASpaceStationCharacter::UpdateStamina(float DeltaTime)
{
	bool bIsMoving = GetVelocity().SizeSquared2D() > 10.0f;

	if (bIsSprinting && !bIsAiming && bIsMoving)
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