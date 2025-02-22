/*
 * Copyright 2022 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "jni_bind.h"

namespace {

using ::jni::Array;
using ::jni::Class;
using ::jni::Constructor;
using ::jni::Field;
using ::jni::Id;
using ::jni::IdType;
using ::jni::JniT;
using ::jni::JniTSelector;
using ::jni::kNoIdx;
using ::jni::Method;
using ::jni::Overload;
using ::jni::Params;
using ::jni::Rank;
using ::jni::Return;
using ::jni::SelectorStaticInfo;
using ::jni::Signature_v;

static constexpr Class kClass1{
    "kClass1",
    Constructor{},
    Constructor{jint{}},
    Constructor{jfloat{}, jboolean{}},
    Constructor{Array{jint{}}},
    Constructor{Array{jint{}, Rank<2>{}}},
    Method{"m0", jni::Return<void>{}, Params{}},
    Method{"m1", jni::Return<jshort>{}, Params<jint>{}},
    Method{"m2", jni::Return{Class{"kClass2"}}, Params<jfloat, jboolean>{}},
    Method{
        "m3",
        Overload{jni::Return<void>{}, Params{}},
        Overload{jni::Return<jint>{}, Params<jboolean>{}},
        Overload{jni::Return<jfloat>{}, Params<jshort, jdouble>{}},
        Overload{jni::Return<jfloat>{}, Params{Class{"kClass2"}}},
        Overload{jni::Return{Class{"kClass3"}}, Params{}},
    },
    Method{
        "m4",
        Overload{jni::Return{Array{jboolean{}}}, Params{Array{jint{}}}},
        Overload{jni::Return{Array<jboolean, 2>{}}, Params{Array<jfloat, 2>{}}},
        Overload{jni::Return{Array<jboolean, 3>{}}, Params{Array<jshort, 3>{}}},
        Overload{jni::Return{Array{Class{"kClass2"}, Rank<2>{}}}, Params{}},
    },
    Field{"f0", int{}},
    Field{"f1", Class{"kClass2"}}};

using JT = JniT<jobject, kClass1>;

////////////////////////////////////////////////////////////////////////////////
// Class (i.e. self).
////////////////////////////////////////////////////////////////////////////////
using kSelf1 = Id<JT, IdType::CLASS, kNoIdx, 0>;
using JniSelfT = JniTSelector<jni::LocalArray<jobject, 3, kClass1>::JniT_>;
using StaticSelectorInfoSelf = SelectorStaticInfo<JniSelfT>;

static_assert(std::string_view{"LkClass1;"} ==
              Signature_v<Id<JT, IdType::CLASS>>);
static_assert(std::string_view{"[[LkClass1;"} ==
              StaticSelectorInfoSelf::TypeName());

////////////////////////////////////////////////////////////////////////////////
// Constructors.
////////////////////////////////////////////////////////////////////////////////
using kCtor0 = Id<JT, IdType::OVERLOAD, kNoIdx, 0>;
using kCtor1 = Id<JT, IdType::OVERLOAD, kNoIdx, 1>;
using kCtor2 = Id<JT, IdType::OVERLOAD, kNoIdx, 2>;
using kCtor3 = Id<JT, IdType::OVERLOAD, kNoIdx, 3>;
using kCtor4 = Id<JT, IdType::OVERLOAD, kNoIdx, 4>;

static_assert(std::string_view{"I"} ==
              Signature_v<Id<JT, IdType::OVERLOAD_PARAM, kNoIdx, 1, 0>>);
static_assert(std::string_view{"F"} ==
              Signature_v<Id<JT, IdType::OVERLOAD_PARAM, kNoIdx, 2, 0>>);
static_assert(std::string_view{"Z"} ==
              Signature_v<Id<JT, IdType::OVERLOAD_PARAM, kNoIdx, 2, 1>>);
static_assert(std::string_view{"[I"} ==
              Signature_v<Id<JT, IdType::OVERLOAD_PARAM, kNoIdx, 3, 0>>);

static_assert(std::string_view{"()V"} == Signature_v<kCtor0>);
static_assert(std::string_view{"(I)V"} == Signature_v<kCtor1>);
static_assert(std::string_view{"(FZ)V"} == Signature_v<kCtor2>);
static_assert(std::string_view{"([I)V"} == Signature_v<kCtor3>);
static_assert(std::string_view{"([[I)V"} == Signature_v<kCtor4>);

////////////////////////////////////////////////////////////////////////////////
// Methods (Overload sets with only one overload).
////////////////////////////////////////////////////////////////////////////////
static_assert(std::string_view{"S"} ==
              Signature_v<Id<JT, IdType::OVERLOAD_PARAM, 1, 0>>);
static_assert(std::string_view{"I"} ==
              Signature_v<Id<JT, IdType::OVERLOAD_PARAM, 1, 0, 0>>);
static_assert(std::string_view{"LkClass2;"} ==
              Signature_v<Id<JT, IdType::OVERLOAD_PARAM, 2, 0, kNoIdx>>);
static_assert(std::string_view{"F"} ==
              Signature_v<Id<JT, IdType::OVERLOAD_PARAM, 2, 0, 0>>);
static_assert(std::string_view{"Z"} ==
              Signature_v<Id<JT, IdType::OVERLOAD_PARAM, 2, 0, 1>>);

static_assert(std::string_view{"V"} ==
              Signature_v<Id<JT, IdType::OVERLOAD_PARAM, 0, 0>>);
static_assert(std::string_view{"S"} ==
              Signature_v<Id<JT, IdType::OVERLOAD_PARAM, 1, 0>>);
static_assert(std::string_view{"I"} ==
              Signature_v<Id<JT, IdType::OVERLOAD_PARAM, 1, 0, 0>>);
static_assert(std::string_view{"LkClass2;"} ==
              Signature_v<Id<JT, IdType::OVERLOAD_PARAM, 2, 0, kNoIdx>>);
static_assert(std::string_view{"F"} ==
              Signature_v<Id<JT, IdType::OVERLOAD_PARAM, 2, 0, 0>>);
static_assert(std::string_view{"Z"} ==
              Signature_v<Id<JT, IdType::OVERLOAD_PARAM, 2, 0, 1>>);

using kMethod0Overload0 = Id<JT, IdType::OVERLOAD, 0, 0>;
using kMethod1Overload0 = Id<JT, IdType::OVERLOAD, 1, 0>;
using kMethod2Overload0 = Id<JT, IdType::OVERLOAD, 2, 0>;
using kMethod3Overload0 = Id<JT, IdType::OVERLOAD, 3, 0>;

static_assert(std::string_view{"()V"} == Signature_v<kMethod0Overload0>);
static_assert(std::string_view{"(I)S"} == Signature_v<kMethod1Overload0>);
static_assert(std::string_view{"(FZ)LkClass2;"} ==
              Signature_v<kMethod2Overload0>);

////////////////////////////////////////////////////////////////////////////////
// Overloaded Method (smoke test, they should behave the same).
////////////////////////////////////////////////////////////////////////////////
using kMethod3Overload0 = Id<JT, IdType::OVERLOAD, 3, 0>;
using kMethod3Overload1 = Id<JT, IdType::OVERLOAD, 3, 1>;
using kMethod3Overload2 = Id<JT, IdType::OVERLOAD, 3, 2>;
using kMethod3Overload3 = Id<JT, IdType::OVERLOAD, 3, 3>;
using kMethod3Overload4 = Id<JT, IdType::OVERLOAD, 3, 4>;

using kMethod4Overload0 = Id<JT, IdType::OVERLOAD, 4, 0>;
using kMethod4Overload1 = Id<JT, IdType::OVERLOAD, 4, 1>;
using kMethod4Overload2 = Id<JT, IdType::OVERLOAD, 4, 2>;
using kMethod4Overload3 = Id<JT, IdType::OVERLOAD, 4, 3>;

static_assert(std::string_view{"()V"} == Signature_v<kMethod3Overload0>);
static_assert(std::string_view{"(Z)I"} == Signature_v<kMethod3Overload1>);
static_assert(std::string_view{"(SD)F"} == Signature_v<kMethod3Overload2>);
static_assert(std::string_view{"(LkClass2;)F"} ==
              Signature_v<kMethod3Overload3>);
static_assert(std::string_view{"()LkClass3;"} ==
              Signature_v<kMethod3Overload4>);

static_assert(std::string_view{"([I)[Z"} == Signature_v<kMethod4Overload0>);
static_assert(std::string_view{"([[F)[[Z"} == Signature_v<kMethod4Overload1>);
static_assert(std::string_view{"([[[S)[[[Z"} == Signature_v<kMethod4Overload2>);
static_assert(std::string_view{"()[[LkClass2;"} ==
              Signature_v<kMethod4Overload3>);

////////////////////////////////////////////////////////////////////////////////
// Fields (Overload sets with only one overload).
////////////////////////////////////////////////////////////////////////////////
using kField0 = Id<JT, IdType::FIELD, 0>;
using kField1 = Id<JT, IdType::FIELD, 1>;

static_assert(std::string_view{"I"} == Signature_v<kField0>);
static_assert(std::string_view{"LkClass2;"} == Signature_v<kField1>);

}  // namespace
