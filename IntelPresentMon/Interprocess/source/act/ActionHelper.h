#pragma once
#include "AsyncAction.h"
#include "../../../CommonUtilities/Macro.h"
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/unordered_set.hpp>

#ifndef PM_ASYNC_ACTION_CUSTOM_REG_
#define PM_ASYNC_ACTION_CUSTOM_REG_(name)
#endif

// the custom hook for action registration seems to be difficult in practice
// this hook mechanism might be removed
#ifdef PM_ASYNC_ACTION_REGISTRATION_
#include "AsyncActionCollection.h"
#define ACTION_REG() ::pmon::ipc::act::AsyncActionRegistrator<ACT_NS::ACT_NAME, ACT_NS::ACT_EXEC_CTX> CONCATENATE(regSvcAct_, ACT_NAME)##_; PM_ASYNC_ACTION_CUSTOM_REG_(ACT_NAME)
#else
#define ACTION_REG() PM_ASYNC_ACTION_CUSTOM_REG_(ACT_NAME)
#endif

#define ACTION_TRAITS_DEF() namespace pmon::ipc::act { template<> struct ActionParamsTraits<ACT_NS::ACT_NAME::Params> { using Action = ACT_NS::ACT_NAME; }; }