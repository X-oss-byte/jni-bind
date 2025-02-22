/*
 * Copyright 2021 Google LLC
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
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "implementation/jni_helper/jni_env.h"
#include "implementation/jni_helper/jni_helper.h"
#include "jni_bind.h"
#include "jni_test.h"

namespace {

using ::jni::Class;
using ::jni::ClassLoader;
using ::jni::GlobalObject;
using ::jni::Jvm;
using ::jni::JvmRef;
using ::jni::kNullClassLoader;
using ::jni::LocalObject;
using ::jni::PromoteToGlobal;
using ::jni::SupportedClassSet;
using ::jni::ThreadGuard;
using ::jni::test::AsGlobal;
using ::jni::test::Fake;
using ::jni::test::JniTest;
using ::jni::test::JniTestWithNoDefaultJvmRef;
using ::jni::test::MockJvm;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Return;

TEST(Jni, JvmStaticClassLoaderLookupsWork) {
  static constexpr Class kClass1{"com/google/Class1"};
  static constexpr Class kClass2{"com/google/Class2"};
  static constexpr Class kClass3{"com/google/Class3"};
  static constexpr ClassLoader kClassLoader1{kNullClassLoader,
                                             SupportedClassSet{kClass1}};
  static constexpr ClassLoader kClassLoader2{
      kNullClassLoader, SupportedClassSet{kClass1, kClass2}};
  static constexpr ClassLoader kClassLoader3{
      kNullClassLoader, SupportedClassSet{kClass1, kClass2, kClass3}};
  static constexpr ClassLoader kUnusedLoader{kNullClassLoader,
                                             SupportedClassSet{}};

  static constexpr Jvm kJvm1{kClassLoader1};
  static_assert(kJvm1.IdxOfClassLoader<kUnusedLoader>() == -1);
  static_assert(kJvm1.IdxOfClassLoader<kClassLoader1>() == 0);

  static constexpr Jvm kJvm2{kClassLoader1, kClassLoader2};
  static_assert(kJvm2.IdxOfClassLoader<kUnusedLoader>() == -1);
  static_assert(kJvm2.IdxOfClassLoader<kClassLoader1>() == 0);
  static_assert(kJvm2.IdxOfClassLoader<kClassLoader2>() == 1);

  static constexpr Jvm kJvm3{kClassLoader1, kClassLoader2, kClassLoader3};
  static_assert(kJvm3.IdxOfClassLoader<kUnusedLoader>() == -1);
  static_assert(kJvm3.IdxOfClassLoader<kClassLoader1>() == 0);
  static_assert(kJvm3.IdxOfClassLoader<kClassLoader2>() == 1);
  static_assert(kJvm3.IdxOfClassLoader<kClassLoader3>() == 2);
}

TEST_F(JniTest, JvmRefTearsDownClassesLoadedfromDefaultLoader) {
  // Note, this expectation means FindClass is called *exactly* once.
  // Using offset to avoid Fake<jclass>() usage in jni_test.h.
  EXPECT_CALL(*env_, FindClass).WillOnce(Return(Fake<jclass>(1)));
  EXPECT_CALL(*env_, DeleteGlobalRef).Times(AnyNumber());
  EXPECT_CALL(*env_, DeleteGlobalRef(AsGlobal(Fake<jclass>(1))));

  static constexpr Class kClass1{"com/google/Class1"};
  LocalObject<kClass1> local_object1{};
}

TEST_F(JniTest, NoStaticCrossTalkWithPriorTest) {
  // Note, this expectation means FindClass is called *exactly* once.
  // Using 1 offset to avoid Fake<jclass>() usage in jni_test.h.
  EXPECT_CALL(*env_, FindClass).WillOnce(Return(Fake<jclass>(1)));
  EXPECT_CALL(*env_, DeleteGlobalRef).Times(AnyNumber());
  EXPECT_CALL(*env_, DeleteGlobalRef(AsGlobal(Fake<jclass>(1))));

  static constexpr Class kClass1{"com/google/Class1"};
  LocalObject<kClass1> local_object1{};
}

TEST_F(JniTest, NoStaticCrossTalkWithUnrelatedTest) {
  // Note, this expectation means FindClass is called *exactly* once.
  // Using 1 offset to avoid Fake<jclass>() usage in jni_test.h.
  EXPECT_CALL(*env_, FindClass).WillOnce(Return(Fake<jclass>(1)));
  EXPECT_CALL(*env_, DeleteGlobalRef).Times(AnyNumber());
  EXPECT_CALL(*env_, DeleteGlobalRef(AsGlobal(Fake<jclass>(1))));

  static constexpr Class kClass1{"com/google/Class2"};
  LocalObject<kClass1> local_object1{};
}

TEST_F(JniTestWithNoDefaultJvmRef,
       JvmsNeitherQueryNorReleaseIfNoObjectsCreated) {
  JvmRef<jni::kDefaultJvm> jvm_ref(jvm_.get());
  EXPECT_CALL(*env_, FindClass).Times(0);
  EXPECT_CALL(*env_, DeleteGlobalRef).Times(0);
}

TEST_F(JniTestWithNoDefaultJvmRef, JvmRefsDontReuuseStaleFindClassValues) {
  EXPECT_CALL(*env_, FindClass)
      .WillOnce(Return(Fake<jclass>(1)))
      .WillOnce(Return(Fake<jclass>(2)));
  EXPECT_CALL(*env_, DeleteGlobalRef).Times(AnyNumber());
  EXPECT_CALL(*env_, DeleteGlobalRef(AsGlobal(Fake<jclass>(1))));
  EXPECT_CALL(*env_, DeleteGlobalRef(AsGlobal(Fake<jclass>(2))));

  static constexpr Class kClass1{"com/google/ADifferentClassForVariety"};
  {
    JvmRef<jni::kDefaultJvm> jvm_ref(jvm_.get());
    LocalObject<kClass1> local_object1{};
  }

  {
    JvmRef<jni::kDefaultJvm> jvm_ref(jvm_.get());
    LocalObject<kClass1> local_object1{};
  }
}

TEST_F(JniTest, DefaultLoaderReleasesMultipleClasses) {
  EXPECT_CALL(*env_, FindClass)
      .WillOnce(Return(Fake<jclass>(1)))
      .WillOnce(Return(Fake<jclass>(2)))
      .WillOnce(Return(Fake<jclass>(3)));
  EXPECT_CALL(*env_, DeleteGlobalRef).Times(AnyNumber());
  EXPECT_CALL(*env_, DeleteGlobalRef(AsGlobal(Fake<jclass>(1))));
  EXPECT_CALL(*env_, DeleteGlobalRef(AsGlobal(Fake<jclass>(2))));
  EXPECT_CALL(*env_, DeleteGlobalRef(AsGlobal(Fake<jclass>(3))));

  static constexpr Class kClass1{"com/google/Class1"};
  static constexpr Class kClass2{"com/google/Class2"};
  static constexpr Class kClass3{"com/google/Class3"};

  LocalObject<kClass1> local_object1{};
  LocalObject<kClass2> local_object2{};
  LocalObject<kClass3> local_object3{};
}

////////////////////////////////////////////////////////////////////////////////
// ThreadGuard Tests.
////////////////////////////////////////////////////////////////////////////////

TEST(ThreadGuard,
     NeverCallsAttachOrDetachCurrentThreadIfAnEnvIsAlreadyAttached) {
  MockJvm jvm;
  EXPECT_CALL(jvm, GetEnv).WillOnce(Return(JNI_OK));
  EXPECT_CALL(jvm, AttachCurrentThread).Times(0);
  EXPECT_CALL(jvm, DetachCurrentThread).Times(0);

  JvmRef<jni::kDefaultJvm> jvm_ref{&jvm};
}

TEST(ThreadGuard, WontEffectDetachmentForPreexistingEnv) {
  MockJvm jvm;
  EXPECT_CALL(jvm, GetEnv).WillOnce(Return(JNI_OK));
  EXPECT_CALL(jvm, AttachCurrentThread).Times(0);
  EXPECT_CALL(jvm, DetachCurrentThread).Times(0);

  JvmRef<jni::kDefaultJvm> jvm_ref{&jvm};
  ThreadGuard thread_guard_1 = jvm_ref.BuildThreadGuard();
  ThreadGuard thread_guard_2 = jvm_ref.BuildThreadGuard();
  ThreadGuard thread_guard_3 = jvm_ref.BuildThreadGuard();
}

// This behaviour is no longer tested! To guarantee GlobalObjects passed into
// lambdas are supported, this happens on thread teardown, which is past
// a point when it can be tested on main thread in unit testing.
/*
TEST(ThreadGuard, CallsAttachCurrentThreadIfEnvIsNotAttached) {
  MockJvm jvm;
  EXPECT_CALL(jvm, GetEnv).WillRepeatedly(Return(JNI_EDETACHED));
  EXPECT_CALL(jvm, AttachCurrentThread).Times(1);
  EXPECT_CALL(jvm, DetachCurrentThread).Times(1);

  JvmRef<jni::kDefaultJvm> jvm_ref{&jvm};
}

TEST(JvmThreadGuard, DetachesOnceForTheMainJvmRefAndThreadGuard) {
  jni::test::MockJvm jvm;

  EXPECT_CALL(jvm, GetEnv).WillRepeatedly(Return(JNI_EDETACHED));
  EXPECT_CALL(jvm, AttachCurrentThread(_, _)).Times(1);
  EXPECT_CALL(jvm, DetachCurrentThread).Times(1);

  // Will call AttachCurrentThread once for the main thread (JvmRef).
  // Subsequent thread guards have no effect.
  JvmRef<jni::kDefaultJvm> jvm_ref{&jvm};
  ThreadGuard thread_guard_1 = jvm_ref.BuildThreadGuard();
}
*/

constexpr jni::Class kObjectTestHelperClass{
    "com/jnibind/test/ObjectTestHelper",

    ::jni::Field{"intVal1", int{}},
};

TEST_F(JniTest, AllowsMoveCtorIntoLambdaWithThreadGuardUsage) {
  GlobalObject<kObjectTestHelperClass> global_obj{PromoteToGlobal{},
                                                  Fake<jobject>(1)};

  std::thread worker{[gobj{std::move(global_obj)}]() mutable {
    ThreadGuard thread_guard{};
    gobj["intVal1"].Get();
  }};

  worker.join();
}

TEST(JvmThreadGuard, DetachesOnceForMultipleGuardsOnSingleThread) {
  jni::test::MockJvm jvm;

  EXPECT_CALL(jvm, GetEnv(_, _)).WillRepeatedly(Return(JNI_EDETACHED));
  EXPECT_CALL(jvm, AttachCurrentThread(_, _)).Times(1);

  // See above uncommented test.
  // EXPECT_CALL(jvm, DetachCurrentThread).Times(1);

  // Will call AttachCurrentThread once for the main thread (JvmRef) and once
  // for the constructed ThreadGuard.
  JvmRef<jni::kDefaultJvm> jvm_ref{&jvm};
  ThreadGuard thread_guard_1 = jvm_ref.BuildThreadGuard();
  ThreadGuard thread_guard_2 = jvm_ref.BuildThreadGuard();
  ThreadGuard thread_guard_3 = jvm_ref.BuildThreadGuard();
  ThreadGuard thread_guard_4 = jvm_ref.BuildThreadGuard();
  ThreadGuard thread_guard_5 = jvm_ref.BuildThreadGuard();
}

TEST(JvmThreadGuard, UpdatesIndividualThreadsWithNewValues) {
  jni::test::MockJvm jvm;

  // This sequence of expectation mimics a normal application.
  // A main thread usually exists with a JNIEnv that is attached on your
  // behalf, and subsequent thread spawns require an explicit attach/detach.
  EXPECT_CALL(jvm, GetEnv(_, _))
      .WillOnce(testing::Invoke([&](void** out_env, int) {
        *out_env = reinterpret_cast<void*>(0xAAAAAAAAAAA);
        return JNI_OK;
      }))
      .WillRepeatedly(Return(JNI_EDETACHED));
  EXPECT_CALL(jvm, AttachCurrentThread(_, _))
      .WillOnce(testing::Invoke([&](void** out_env, void*) {
        *out_env = reinterpret_cast<void*>(0xBBBBBBBBBBB);
        return JNI_OK;
      }))
      .WillOnce(testing::Invoke([&](void** out_env, void*) {
        *out_env = reinterpret_cast<void*>(0xCCCCCCCCCCC);
        return JNI_OK;
      }));
  EXPECT_CALL(jvm, DetachCurrentThread()).Times(2);

  JvmRef<jni::kDefaultJvm> jvm_ref{&jvm};

  std::mutex mutex;
  std::vector<JNIEnv*> observed_envs;

  // Each thread will have a JNIEnv that is thread local and unset until the
  // ThreadGuard is present and sets it.
  std::thread test_thread_1([&]() {
    EXPECT_EQ(jni::JniEnv::GetEnv(), nullptr);
    ThreadGuard thread_guard_1 = jvm_ref.BuildThreadGuard();

    std::lock_guard<std::mutex> lock{mutex};
    observed_envs.push_back(jni::JniEnv::GetEnv());
  });
  std::thread test_thread_2([&]() {
    // Note, multiple thread guards are permitted, but they have no effect.
    EXPECT_EQ(jni::JniEnv::GetEnv(), nullptr);
    ThreadGuard thread_guard_1 = jvm_ref.BuildThreadGuard();
    ThreadGuard thread_guard_2 = jvm_ref.BuildThreadGuard();
    ThreadGuard thread_guard_3 = jvm_ref.BuildThreadGuard();
    ThreadGuard thread_guard_4 = jvm_ref.BuildThreadGuard();

    std::lock_guard<std::mutex> lock{mutex};
    observed_envs.push_back(jni::JniEnv::GetEnv());
  });

  test_thread_1.join();
  test_thread_2.join();

  // Placing EXPECT_EQ calls within the thread lambdas caused unexpected data
  // races.  Given the first thread is joined and then the second, it would seem
  // impossible for this to happen, however, EXPECT_EQ doesn't make guarantees
  // about the ordering of expectations (whereas join does). The write guard is
  // simply to ensure writes are seen on the main thread (this is all but
  // guaranteed to happen anyways, but this is technically more friendly to the
  // compiler).
  //
  // Also, detachment is in TLS destruction, which isn't deterministic, however,
  // what matters is that both complete, not really the order.
  const std::vector<JNIEnv*> expected_output_1{
      reinterpret_cast<JNIEnv*>(0xBBBBBBBBBBB),
      reinterpret_cast<JNIEnv*>(0xCCCCCCCCCCC)};
  const std::vector<JNIEnv*> expected_output_2{
      reinterpret_cast<JNIEnv*>(0xCCCCCCCCCCC),
      reinterpret_cast<JNIEnv*>(0xBBBBBBBBBBB)};
  EXPECT_TRUE((observed_envs == expected_output_1 ||
               observed_envs == expected_output_2));
}

}  // namespace
