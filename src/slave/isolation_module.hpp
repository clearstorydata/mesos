/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __ISOLATION_MODULE_HPP__
#define __ISOLATION_MODULE_HPP__

#include <string>

#include <mesos/mesos.hpp>

#include <process/process.hpp>

#include "common/resources.hpp"

#include "slave/flags.hpp"

namespace mesos {
namespace internal {
namespace slave {

// Forward declaration.
class Slave;


class IsolationModule : public process::Process<IsolationModule>
{
public:
  static IsolationModule* create(const std::string& type);
  static void destroy(IsolationModule* module);

  virtual ~IsolationModule() {}

  // Called during slave initialization.
  virtual void initialize(const Flags& flags,
                          bool local,
                          const process::PID<Slave>& slave) = 0;

  // Called by the slave to launch an executor for a given framework.
  virtual void launchExecutor(const FrameworkID& frameworkId,
                              const FrameworkInfo& frameworkInfo,
                              const ExecutorInfo& executorInfo,
                              const std::string& directory,
                              const Resources& resources) = 0;

  // Terminate a framework's executor, if it is still running.
  // The executor is expected to be gone after this method exits.
  virtual void killExecutor(const FrameworkID& frameworkId,
                            const ExecutorID& executorId) = 0;

  // Update the resource limits for a given framework. This method will
  // be called only after an executor for the framework is started.
  virtual void resourcesChanged(const FrameworkID& frameworkId,
                                const ExecutorID& executorId,
                                const Resources& resources) = 0;
};

} // namespace slave {
} // namespace internal {
} // namespace mesos {

#endif // __ISOLATION_MODULE_HPP__
