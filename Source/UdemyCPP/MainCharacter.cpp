// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/world.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Weapon.h"
#include "Runtime/Engine/Public/TimerManager.h"
#include "UdemyCPP/CustomCharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"



// Sets default values

// We also need to make sure the Character class uses our newly created component instead of the base one:
AMainCharacter::AMainCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCustomCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	// this can be turned off
	PrimaryActorTick.bCanEverTick = true;

	// cache the cast and retrieve it with a function:
	MovementComponent = Cast<UCustomCharacterMovementComponent>(GetCharacterMovement());

	/** Create Camera Boom (pulls towards the player if there is a collision) */
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 600.f; // Camera Follows at this distance
	CameraBoom->bUsePawnControlRotation = true; // Rotate arm based on controller

	/** Create Follow Camera */
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Follow Camera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	/** Attach the camera to the end of the boom and let the boom adjust to match 
	the controller orientation */
	FollowCamera->bUsePawnControlRotation = false;

	// Set Our turn rates for input
	BaseTurnRate = 65.f;
	BaseLookUpRate = 65.f;

	// dont rotate when the controller rotates. Let that just affect the camera
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// Configure Character Movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.f, 0.f); // ...at this rotation rate (speed)
	GetCharacterMovement()->JumpZVelocity = 650.f;
	GetCharacterMovement()->AirControl = 0.2f;

	/** Since we are not going to change ofter the collision capsule size, we can hardcode it */
	GetCapsuleComponent()->SetCapsuleSize(34.f, 88.f);

	MaxHealth = 100.f;
	Health = 65.f;
	MaxStamina = 150.f;
	Stamina = 120.f;
	Coins = 0;
	RunningSpeed = 650.f;
	SprintingSpeed = 950.f;
	ExhaustedSpeed = 400.f;
	DashDistance = 6000.f;
	bShiftKeyDown = false;
	bLMBDown = false;
	bCanDash = true;

	// Initialize Enums
	MovementStatus = EMovementStatus::EMS_Normal;
	StaminaStatus = EStaminaStatus::ESS_Normal;

	StaminaDrainRate = 25.f;
	MinSprintStamina = 50.f;

	

}



void AMainCharacter::DecrementHealth(float Amount)
{
	if (Health - Amount <= 0.f) 
	{
		Health -= Amount;
		Die();
	}
	else 
	{
		Health -= Amount;
	}
}

void AMainCharacter::Die()
{
}

void AMainCharacter::IncrementCoins(int32 Amount)
{
	Coins += Amount;
}

// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

}


// Called every frame
void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float DeltaStamina = StaminaDrainRate * DeltaTime; // how much the stamina should change in this particular frame in order to drain or recover

	// Make Bool that shows if character can start sprint bCanStartSprint and check if Velocity != 0

	switch (StaminaStatus) 
	{
	case EStaminaStatus::ESS_Normal:
		if (bShiftKeyDown && GetCharacterMovement()->Velocity != FVector::ZeroVector)
		{
			if (Stamina - DeltaStamina <= MinSprintStamina)
			{
				SetStaminaStatus(EStaminaStatus::ESS_BelowMinimum);
				Stamina -= DeltaStamina;
			}
			else
			{
				Stamina -= DeltaStamina;
			}
			SetMovementStatus(EMovementStatus::EMS_Sprinting);
		}
		else // Shift key up
		{
			if (Stamina + DeltaStamina >= MaxStamina)
			{
				Stamina = MaxStamina; // we don't want to increment the current stamina above the max stamina of the player
			}
			else
			{
				Stamina += DeltaStamina;
				SetMovementStatus(EMovementStatus::EMS_Normal);
			}
		}
		break;
	case EStaminaStatus::ESS_BelowMinimum:
		if (bShiftKeyDown && GetCharacterMovement()->Velocity != FVector::ZeroVector)
		{
			if (Stamina - DeltaStamina <= 0.f)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Exhausted);
				Stamina = 0;
				SetMovementStatus(EMovementStatus::EMS_Normal);
			} 
			else
			{
				Stamina -= DeltaStamina;
				SetMovementStatus(EMovementStatus::EMS_Sprinting);
			}
		}
		else // shift key up
		{
			if (Stamina + DeltaStamina >= MinSprintStamina)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Normal);
				Stamina += DeltaStamina;
			}
			else
			{
				Stamina += DeltaStamina;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
		}

		break;
	case EStaminaStatus::ESS_Exhausted:
		if (bShiftKeyDown)
		{
			Stamina = 0.f;
			
			
		}
		else // shift key up
		{
			SetStaminaStatus(EStaminaStatus::ESS_ExhaustedRecovering);
			Stamina += DeltaStamina;
			
		}
		SetMovementStatus(EMovementStatus::EMS_ExhaustedWalking);
		break;
	case EStaminaStatus::ESS_ExhaustedRecovering:
		if (Stamina + DeltaStamina >= MinSprintStamina)
		{
			SetStaminaStatus(EStaminaStatus::ESS_Normal);
			Stamina += DeltaStamina;
		}
		else
		{
			Stamina += DeltaStamina;
		}
		SetMovementStatus(EMovementStatus::EMS_ExhaustedWalking);
		break;
	default:
		; // empty statement
	}

}


// Called to bind functionality to input
void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMainCharacter::ShiftKeyDown);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMainCharacter::ShiftKeyUp);

	PlayerInputComponent->BindAction("Climb", IE_Released, this, &AMainCharacter::Climb);
	PlayerInputComponent->BindAction("Climb Exit State", IE_Pressed, this, &AMainCharacter::CancelClimb);

	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &AMainCharacter::Dashing);

	PlayerInputComponent->BindAction("LMB", IE_Pressed, this, &AMainCharacter::LMBDown);
	PlayerInputComponent->BindAction("LMB", IE_Released, this, &AMainCharacter::LMBUp);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMainCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMainCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMainCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMainCharacter::LookUpAtRate);
		
}	

void AMainCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f) && (!bAttacking)) // prevent us moving around if we are attacking
	{
		FVector Direction;
		if (MovementComponent->IsClimbing())
		{
			Direction = FVector::CrossProduct(MovementComponent->GetClimbSurfaceNormal(), -GetActorRightVector());
		}
		else
		{
			// find out which way is forward
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

			// create a FRotationMatrix from YawRotation.	
			Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			
		}
		AddMovementInput(Direction, Value);
		
    }
}

void AMainCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f) && (!bAttacking)) // prevent us moving around if we are attacking
	{
		FVector Direction;
		if (MovementComponent->IsClimbing())
		{
			Direction = FVector::CrossProduct(MovementComponent->GetClimbSurfaceNormal(), GetActorUpVector());
		}
		else
		{
			// find out which way is forward
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

			// create a FRotationMatrix from YawRotation.	
			Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		}
		
		AddMovementInput(Direction, Value);
	}

}

void AMainCharacter::TurnAtRate(float Rate)
{ 
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMainCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMainCharacter::LMBDown()
{
	bLMBDown = true;
	if (ActiveOverlappingItem)
	{
		AWeapon* Weapon = Cast<AWeapon>(ActiveOverlappingItem);
		if (Weapon) // if the cast succeed
		{
			Weapon->Equip(this);
			SetActiveOverlappingItem(nullptr);
		}
	}
	else if (EquippedWeapon)
	{
		Attack();

	}


}

void AMainCharacter::LMBUp()
{
	bLMBDown = false;
}

void AMainCharacter::ResetDash()
{
	bCanDash = true;
	GetWorldTimerManager().ClearTimer(DashDelay);
}

void AMainCharacter::SetEquippedWeapon(AWeapon* WeaponToSet)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
	}
	EquippedWeapon = WeaponToSet;

}

void AMainCharacter::Attack()
{
	if (!bAttacking)
	{
		bAttacking = true;

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

		if (AnimInstance && CombatMontage)
		{
			int32 Section = FMath::RandRange(0, 1); // choose different attacks
			switch (Section)
			{
			case 0:
				AnimInstance->Montage_Play(CombatMontage, 1.9f);
				AnimInstance->Montage_JumpToSection(FName("Attack_1"), CombatMontage);
				break;
			case 1:
				AnimInstance->Montage_Play(CombatMontage, 1.8f);
				AnimInstance->Montage_JumpToSection(FName("Attack_2"), CombatMontage);
				break;

			default:
				;
			}
			
		}
	}
	}
	

void AMainCharacter::AttackEnd()
{
	bAttacking = false;
	if (bLMBDown)
	{
		Attack();
	}
}

void AMainCharacter::SetMovementStatus(EMovementStatus Status)
{
	MovementStatus = Status; // change the status equal to the movement status that is passed in
	if (MovementStatus == EMovementStatus::EMS_Sprinting) 
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintingSpeed;
	}
	else if (MovementStatus == EMovementStatus::EMS_ExhaustedWalking)
	{
		GetCharacterMovement()->MaxWalkSpeed = ExhaustedSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
	}
	
}

void AMainCharacter::Dashing()
{

	const FVector ForwardDir = this->GetActorRotation().Vector();

	if (Stamina > MinSprintStamina && GetCharacterMovement()->Velocity != FVector::ZeroVector && bCanDash)  {
		if (GetCharacterMovement()->IsMovingOnGround() == false) 
		{
			bCanDash = false;
			LaunchCharacter(ForwardDir * DashDistance / 3, true, false);
			Stamina = Stamina - 40;
			//Change the state if stamina is low
			SetStaminaStatus(EStaminaStatus::ESS_BelowMinimum);
			GetWorld()->GetTimerManager().SetTimer(DashDelay, this, &AMainCharacter::ResetDash, 2.f);
		}
		else 
		{
			bCanDash = false;
			LaunchCharacter(ForwardDir * DashDistance, true, false);
			Stamina = Stamina - 40;
			//Change the state if stamina is low
			SetStaminaStatus(EStaminaStatus::ESS_BelowMinimum);
			GetWorld()->GetTimerManager().SetTimer(DashDelay, this, &AMainCharacter::ResetDash, 2.f);
		}
		

	}
	// TODO: Find a smarter way to fix the dash during air be so slow and "open"
    // TODO: Fix the long dash in air, test all the functionalities (shift + E goes directly into belowMinimum)
}



void AMainCharacter::ShiftKeyDown()
{
	bShiftKeyDown = true;
}

void AMainCharacter::ShiftKeyUp()
{
	bShiftKeyDown = false;
}


void AMainCharacter::Climb()
{
	MovementComponent->TryClimbing();
}

void AMainCharacter::CancelClimb()
{
	MovementComponent->CancelClimbing();
}