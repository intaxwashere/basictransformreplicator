// Made by Intax

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "TransformReplicatorComponent.generated.h"

/*
* This is a very basic transform replicator, not suitable for physics and movement.
* This will replicate and smooth your transform difference between server and client.
*/

USTRUCT(BlueprintType)
struct FTRServerData
{
	GENERATED_BODY()
	UPROPERTY()
	FVector_NetQuantize100 Location;

	UPROPERTY()
	FQuat Rotation;

	UPROPERTY()
	FVector_NetQuantize100 Scale;

	UPROPERTY()
	FVector_NetQuantize100 LinearVelocity;

	UPROPERTY()
	FVector_NetQuantize100 AngularVelocity;
};

USTRUCT(BlueprintType)
struct FTRClientData
{
	GENERATED_BODY()
	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FQuat Rotation;

	UPROPERTY()
	FVector Scale;

	UPROPERTY()
	FVector LinearVelocity;

	UPROPERTY()
	FVector AngularVelocity;
};

UENUM(BlueprintType)
enum class ETRInterpMethod : uint8
{
	Automatic		UMETA(DisplayName="Automatic"),
	LinearInterp	UMETA(DisplayName="Linear Interpolation"),
	VectorInterp	UMETA(DisplayName="Vector Interpolation"),
	CubicInterp		UMETA(DisplayName="Cubic Interpolation"),
};

UENUM()
enum ETRCurrentInterpMethod
{
	Automatic,
	CubicInterp,
	VectorInterp
};

UENUM(BlueprintType)
enum ETRInterpSkipReason
{
	DistanceTooBig	UMETA(DisplayName = "Distance is Too Big"),
	UnexpectedError	UMETA(DisplayName = "Unexpected Error"),
};

UCLASS( ClassGroup=(Intax), meta=(BlueprintSpawnableComponent) )
class TRANSFORMREPLICATOR_API UTransformReplicatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTransformReplicatorComponent();

	UPROPERTY(BlueprintReadOnly)
	UPrimitiveComponent* UpdatedComponent;

	/* Replicates & Interpolates Location if enabled. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Component Settings")
	bool bReplicateLocation;

	/* Replicates & Interpolates Rotation if enabled. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Component Settings")
	bool bReplicateRotation;

	/* Replicates & Interpolates Scale if enabled. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Component Settings")
	bool bReplicateScale;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Component Settings")
	bool bReplicateLinearVelocity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Component Settings")
	bool bReplicateAngularVelocity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Component Settings")
	TEnumAsByte<ETRCurrentInterpMethod> CurrentInterpMethod;

	/* Gets the current Updated Component. */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	USceneComponent* GetUpdatedComponent() const { return UpdatedComponent; };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxDistanceForInterp;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ToolTip="If update between old and new positions exceeds this value, component will run CubicInterp."))
	float MinDistanceForVectorInterp;

	UPROPERTY()
	bool bServerSimulatingPhysics;

	UPROPERTY()
	bool bShouldUpdatePhysics;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void ClientTick(float DeltaTime);

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/* If interpolation skipped, this function will be called. */
	UFUNCTION(BlueprintNativeEvent)
	ETRInterpSkipReason OnInterpolationSkipped(ETRInterpSkipReason SkipReason);

private:

	UFUNCTION(Server, Unreliable)
	void Server_SendData();

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FTRServerData ServerData;

	UFUNCTION()
	void OnRep_ServerState();

	// Client values
	FTRClientData ClientData;
	float ClientTimeSinceUpdate;
	float ClientTimeBetweenLastUpdates;
	float ClientSimulatedTime;
		
};
