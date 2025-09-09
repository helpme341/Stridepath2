
#pragma once

#include "CoreMinimal.h"
#include "Misc/InlineValue.h"

enum class EInvokerType { Fun, ConstFun, StaticFunc, Lambda };

template<uint16 DesiredMaxInlineSize = 128, uint8 DefaultAlignment = 8>
struct FInvokerConfig final
{
	static constexpr uint16 DesiredMaxInlineSizeValue = DesiredMaxInlineSize;
	static constexpr uint8 DefaultAlignmentValue = DefaultAlignment;
};

template<typename Signature, typename Config = FInvokerConfig<>>
struct TInvoker;

template<typename RetT, typename... Args, typename Config>
struct TInvoker<RetT(Args...), Config> final
{
	static constexpr uint16 DesiredMaxInlineSize = Config::DesiredMaxInlineSizeValue;
	static constexpr uint8 DefaultAlignment = Config::DefaultAlignmentValue;
	
private:
	struct TBaseHolder
	{
		virtual ~TBaseHolder() = default;
		virtual RetT Invoke(void* RawInstance, Args&&... InArgs) { return RetT(); }

		EInvokerType InvokerType = EInvokerType::Fun;
	};

	template<typename LambdaT>
	struct TSimpleHolder final : TBaseHolder
	{
		explicit TSimpleHolder(const EInvokerType& InType, LambdaT&& InFunc) : Func(std::move(InFunc)) { TBaseHolder::InvokerType = InType; }
		
		FORCEINLINE virtual RetT Invoke(void* RawInstance, Args&&... InArgs) override
		{
			return Func(std::forward<Args>(InArgs)...);
		}
		LambdaT Func;
	};

	template<typename LambdaT>
	struct THolder final : TBaseHolder
	{
		explicit THolder(const EInvokerType& InType, LambdaT&& InFunc) : Func(std::move(InFunc)) { TBaseHolder::InvokerType = InType; }
		explicit THolder(const EInvokerType& InType, const LambdaT& InFunc) : Func(std::move(InFunc)) { TBaseHolder::InvokerType = InType; }
		
		FORCEINLINE virtual RetT Invoke(void* RawInstance, Args&&... InArgs) override
		{
			return Func(RawInstance, std::forward<Args>(InArgs)...);
		}
		LambdaT Func;
	};

	struct THolderContainer final
	{
		TInlineValue<TBaseHolder, DesiredMaxInlineSize, DefaultAlignment> SmallHolder;
		TUniquePtr<TBaseHolder> LargeHolder;
		bool bIsLarge = false;

		template<typename HolderT, typename LambdaT>
		void CreateHolder(const EInvokerType& Type, LambdaT&& InLambda)
		{
			if (sizeof(InLambda) <= DesiredMaxInlineSize)
			{
				SmallHolder = HolderT(Type, MoveTemp(InLambda));
				bIsLarge = false;
			}
			else
			{
				LargeHolder = MakeUnique<HolderT>(Type, MoveTemp(InLambda));
				bIsLarge = true;	
			}
		}

		FORCEINLINE bool IsBound() const { return GetHolder(); }
		FORCEINLINE void ClearHolder()
		{
			if (bIsLarge) LargeHolder.Reset();
			else SmallHolder.Reset();
		}
	 	FORCEINLINE TBaseHolder& GetHolder() { return bIsLarge ? *LargeHolder : *SmallHolder; }
	};

	THolderContainer HolderContainer;
	void* Instance = nullptr;
public:
	
	TInvoker() = default;

	TInvoker(const TInvoker&) = delete;
	TInvoker& operator=(const TInvoker&) = delete;

	TInvoker(TInvoker&& Other) noexcept
		: HolderContainer(MoveTemp(Other.HolderContainer))
		, Instance(Other.Instance)
	{
		Other.Instance = nullptr;
	}

	TInvoker& operator=(TInvoker&& Other) noexcept
	{
		if (this != &Other)
		{
			HolderContainer   = THolderContainer();
			HolderContainer   = MoveTemp(Other.HolderContainer);
			Instance = Other.Instance;
			Other.Instance = nullptr;
		}
		return *this;
	}

	template<typename ClassType>
	FORCEINLINE TInvoker(ClassType* InInstance, RetT(ClassType::*InMethod)(Args...)) { Bind(InInstance, InMethod); }

	template<typename ClassType>
	FORCEINLINE TInvoker(ClassType* InInstance, RetT(ClassType::*InMethod)(Args...) const) { Bind(InInstance, InMethod); }

	FORCEINLINE TInvoker(RetT(*StaticFunc)(Args...)) { Bind(StaticFunc); }
	
	template<typename LambdaT>
	FORCEINLINE TInvoker(LambdaT&& Lambda) { Bind(Lambda); }
		
	FORCEINLINE RetT operator()(Args... InArgs)
	{	
		return HolderContainer.GetHolder().Invoke(Instance, std::forward<Args>(InArgs)...);
	}

	FORCEINLINE bool operator==(TInvoker&& Another) const
	{
		return Another.Instance == Instance && Another.HolderContainer == HolderContainer;
	}

	FORCEINLINE explicit operator bool() const { return IsBound(); }

	
	template<typename ClassType>
	void Bind(ClassType* InInstance, RetT(ClassType::*InMethod)(Args...))
	{
		auto Lambda = [InMethod](void* RawInstance, Args&&...InArgs)
		{
			ClassType* TypedInstance = static_cast<ClassType*>(RawInstance);
			return (TypedInstance->*InMethod)(std::forward<Args>(InArgs)...);
		};
		
		Unbind();
		Instance = static_cast<void*>(InInstance);
		HolderContainer.template CreateHolder<THolder<decltype(Lambda)>>(EInvokerType::Fun, Lambda);
	}
 
	template<typename ClassType>
	void Bind(ClassType* InInstance, RetT(ClassType::*InMethod)(Args...) const)
	{
		auto Lambda = [InMethod](void* RawInstance, Args&&...InArgs)
		{
			ClassType* TypedInstance = static_cast<ClassType*>(RawInstance);
			return (TypedInstance->*InMethod)(std::forward<Args>(InArgs)...);
		};
			
		Unbind();
		Instance = static_cast<void*>(InInstance);
		HolderContainer.template CreateHolder<THolder<decltype(Lambda)>>(EInvokerType::ConstFun, Lambda);
	}
	
	FORCEINLINE void Bind(RetT(*StaticFunc)(Args...))
	{
		auto Lambda = [StaticFunc](Args&&...InArgs)
		{
			return StaticFunc(std::forward<Args>(InArgs)...);
		};
		
		Unbind();
		HolderContainer.template CreateHolder<TSimpleHolder<decltype(Lambda)>>(EInvokerType::StaticFunc, Lambda);
	}

	template<typename LambdaT>
	void Bind(LambdaT&& Lambda)
	{
		Instance = nullptr;
		HolderContainer.template CreateHolder<TSimpleHolder<std::decay_t<LambdaT>>>(EInvokerType::Lambda, Lambda);
	}

	FORCEINLINE void Unbind()
	{
		Instance = nullptr;
		HolderContainer.ClearHolder();
	}

	FORCEINLINE EInvokerType GetInvokerType() const { return HolderContainer.GetGetHolder()->InvokerType; }
	FORCEINLINE bool IsBound() const
	{
		return HolderContainer.IsBound();
	}
};