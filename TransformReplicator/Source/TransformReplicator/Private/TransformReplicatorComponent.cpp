// Made by Intax

#include "TransformReplicatorComponent.h"
#include "MeshOffsetRoot.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"

#define DEBUGGING_COMPONENT 1

// Sets default values for this component's properties
UTransformReplicatorComponent::UTransformReplicatorComponent()
{
	// Keep this enabled.
	PrimaryComponentTick.bCanEverTick = true;

	MaxDistanceForInterp = 256.f;
	MinDistanceForVectorInterp = 25.f;

	SetIsReplicated(true);
}

void UTransformReplicatorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UTransformReplicatorComponent, ServerData);
}

ETRInterpSkipReason UTransformReplicatorComponent::OnInterpolationSkipped_Implementation(ETRInterpSkipReason SkipReason)
{
	return SkipReason;
}

// Called when the game starts
void UTransformReplicatorComponent::BeginPlay()
{
	Super::BeginPlay();
	
	UpdatedComponent = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
}

// Called every frame
void UTransformReplicatorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(GetOwner()->HasAuthority())
	{
		Server_SendData();
		if(IsValid(UpdatedComponent) && UpdatedComponent->IsSimulatingPhysics())
		{
			bServerSimulatingPhysics = 1;
		}
	}
	else
	{
		ClientTick(DeltaTime);
	}
}

void UTransformReplicatorComponent::Server_SendData_Implementation()
{
	if(!IsValid(UpdatedComponent)) return;
	ServerData.Location = UpdatedComponent->GetComponentTransform().GetLocation();
	ServerData.Rotation = UpdatedComponent->GetComponentTransform().GetRotation();
	ServerData.Scale = UpdatedComponent->GetComponentTransform().GetScale3D();
	ServerData.LinearVelocity = UpdatedComponent->GetPhysicsAngularVelocityInDegrees();
	ServerData.AngularVelocity = UpdatedComponent->GetPhysicsAngularVelocityInDegrees();
}

void UTransformReplicatorComponent::OnRep_ServerState()
{
	if(GetOwnerRole() == ROLE_SimulatedProxy)
	{
		ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
		ClientTimeSinceUpdate = 0;

		if (IsValid(UpdatedComponent))
		{
			ClientData.Location = UpdatedComponent->GetComponentLocation();
			ClientData.Rotation = UpdatedComponent->GetComponentQuat();
			ClientData.Scale = UpdatedComponent->GetComponentScale();
			bShouldUpdatePhysics = bServerSimulatingPhysics;
			ClientData.LinearVelocity = ServerData.LinearVelocity;
			ClientData.AngularVelocity = ServerData.AngularVelocity;

			FTransform Transform;
			Transform.SetLocation(ServerData.Location);
			Transform.SetRotation(ServerData.Rotation);
			Transform.SetScale3D(GetOwner()->GetActorScale3D());
			UpdatedComponent->SetWorldTransform(Transform);
		}
	}
}

void UTransformReplicatorComponent::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

	if(!IsValid(UpdatedComponent)) return;

	if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER) return;
	//if (ClientTimeBetweenLastUpdates > 1.f) return;

	bool bSimulatingPhysics = UpdatedComponent->IsSimulatingPhysics(); // Cache it 
	if(bSimulatingPhysics) UpdatedComponent->SetSimulatePhysics(false); // Lock the physics to server-side.

	const float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;

	// ServerLocation is only for avoiding CubicInterp errors about NetQuantize. (FVector_NetQuantize can not be used on CubicInterp)
	const FVector ServerLocation = ServerData.Location;
	// Get the distance between old(client) and new(server) locations to determine interpolation method.
	float DistanceBetweenVectors = ServerLocation.Size() - ClientData.Location.Size();
	// Sometimes .Size can return negative values, we are asking for Abs value
	DistanceBetweenVectors = FMath::Abs(DistanceBetweenVectors);

	if (DistanceBetweenVectors > MaxDistanceForInterp)
	{
		OnInterpolationSkipped(DistanceTooBig);
		return;
	}

	if (CurrentInterpMethod == Automatic && DistanceBetweenVectors > MinDistanceForVectorInterp)
	{
		CurrentInterpMethod = CubicInterp;
	}
	else
	{
		CurrentInterpMethod = VectorInterp;
	}

	// Linear Interpolation and CubicInterp doesn't like Quantized vectors. So we are converting them to usual FVectors.
	FVector NonQuantizedLoc = ServerData.Location;
	FVector NonQuantizedScale = ServerData.Scale;
	FVector NonQuantizedLinearVel = ServerData.LinearVelocity;
	FVector NonQuantizedAngularVel = ServerData.AngularVelocity;
	FQuat SlerpQuat = ServerData.Rotation; // Only to please the Slerp..

	FVector NewLocation;
	FRotator NewRotation;
	FVector NewScale;
	FVector NewLinearVelocity;
	FVector NewAngularVelocity;

	switch(CurrentInterpMethod)
	{
		case CubicInterp:
			NewLocation = FMath::CubicInterp(ClientData.Location, ClientData.LinearVelocity, NonQuantizedLoc, ClientData.LinearVelocity, LerpRatio);
			SlerpQuat = FQuat::Slerp(ClientData.Rotation, ServerData.Rotation, LerpRatio);
			NewScale = FMath::VInterpTo(ClientData.Scale, ServerData.Scale, DeltaTime, (LerpRatio * 100));
			NewLinearVelocity = FMath::CubicInterpDerivative(ClientData.Location, ClientData.LinearVelocity, NonQuantizedLoc, ClientData.LinearVelocity, LerpRatio);
			NewAngularVelocity = FMath::CubicInterpDerivative(ClientData.Location, ClientData.AngularVelocity, NonQuantizedLoc, ClientData.AngularVelocity, LerpRatio);
			break;

		// Booo! Vector Interpolation sucks!
		case VectorInterp:
			NewLocation = FMath::VInterpTo(ClientData.Location, ServerData.Location, DeltaTime, (LerpRatio * 100));
			NewRotation = FMath::RInterpTo(ClientData.Rotation.Rotator(), ServerData.Rotation.Rotator(), DeltaTime, (LerpRatio * 100));
			NewScale = FMath::VInterpTo(ClientData.Scale, ServerData.Scale, DeltaTime, (LerpRatio * 100));
			NewLinearVelocity = FMath::VInterpTo(ClientData.LinearVelocity, ServerData.LinearVelocity, DeltaTime, (LerpRatio * 100));
			NewAngularVelocity = FMath::VInterpTo(ClientData.AngularVelocity, ServerData.AngularVelocity, DeltaTime, (LerpRatio * 100));
			break;

		default:
			break;
	}
	
	if (IsValid(UpdatedComponent))
	{
		if (bReplicateLocation) UpdatedComponent->SetWorldLocation(NewLocation);
		if (bReplicateRotation)
		{
			if(CurrentInterpMethod == CubicInterp)
			{
				UpdatedComponent->SetWorldRotation(SlerpQuat);
			}
			else
			{
				UpdatedComponent->SetWorldRotation(NewRotation);
			}
		}
		if (bReplicateScale) UpdatedComponent->SetWorldScale3D(NewScale);
		if (bReplicateLinearVelocity && bShouldUpdatePhysics) UpdatedComponent->SetAllPhysicsLinearVelocity(NewLinearVelocity);
		if (bReplicateAngularVelocity && bShouldUpdatePhysics) UpdatedComponent->SetAllPhysicsAngularVelocityInDegrees(NewAngularVelocity);
	}
}




