// Fill out your copyright notice in the Description page of Project Settings.

#pragma once // it prevents to include the same file twice 

#include "CoreMinimal.h" 
#include "UObject/NoExportTypes.h"
#include "MyObject.generated.h"

/**
 *
 */
UCLASS(Blueprintable) // this will allow us to create a blueprint derived from that C++ class
class UDEMYCPP_API UMyObject : public UObject
{
	GENERATED_BODY()
public:
	UMyObject();

	UPROPERTY(BlueprintReadOnly, Category = "My Variables") // this exposes the float variable to the reflection system, BPReadWrite will give access to the variables in our BP. 
		//In order to work it has to be in the public section
	float MyFloat;

    UFUNCTION(BlueprintCallable, Category = "My Functions") // does the same but for the functions
	void MyFunction();
	  


};