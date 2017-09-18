/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "DexClass.h"
#include "DexInstruction.h"
#include "DexUtil.h"
#include "JarLoader.h"
#include "ProguardConfiguration.h"
#include "ProguardParser.h"
#include "ReachableClasses.h"
#include "Tool.h"
#include "Walkers.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

using JarMethodInfoMap = std::unordered_map<
    // Method as string
    std::string,
    // Fields of interest in "Code_attribute" from JVM class file.
    std::tuple<int /*max_stack*/, int /*max_locals*/, int /*code_length*/>>;

JarMethodInfoMap load_jar_method_info(const std::string& base_directory,
                                      const std::vector<std::string>& jars) {
  JarMethodInfoMap info;
  auto hook = [&info](boost::variant<DexField*, DexMethod*> field_or_method,
                      const char* attribute_name,
                      uint8_t* attribute_pointer) {
    // 0: DexField, 1: DexMethod
    if (field_or_method.which() != 1 || strcmp(attribute_name, "Code") != 0) {
      return;
    }

    uint16_t max_stack = JarLoaderUtil::read16(attribute_pointer);
    uint16_t max_locals = JarLoaderUtil::read16(attribute_pointer);
    uint32_t code_length = JarLoaderUtil::read32(attribute_pointer);
    DexMethod* method = boost::get<DexMethod*>(field_or_method);
    info.emplace(show(method),
                 std::make_tuple(max_stack, max_locals, code_length));
  };

  for (const auto& jar : jars) {
    load_jar_file((base_directory + "/" + jar).c_str(), hook);
  }
  return info;
}

void diff_in_out_jars_from_command_line(const std::string& command_line_path) {
  std::ifstream config(command_line_path);
  if (!config.is_open()) {
    std::cerr << "Unable to open \'" << command_line_path << '\'' << std::endl;
    return;
  }

  redex::ProguardConfiguration pg_config;
  redex::proguard_parser::parse(config, &pg_config);
  std::cout << "Number of -injar options: " << pg_config.injars.size()
            << std::endl;
  std::cout << "Number of -outjar options: " << pg_config.outjars.size()
            << std::endl;

  RedexContext* injar_context = g_redex;
  std::cout << "Reading injar files... " << std::flush;
  auto injar_info =
      load_jar_method_info(pg_config.basedirectory, pg_config.injars);
  std::cout << injar_info.size() << " method info loaded." << std::endl;

  // Create a new context for outjars and switch the context.
  std::unique_ptr<RedexContext> outjar_context(new RedexContext());
  g_redex = outjar_context.get();
  std::cout << "Reading outjar files... " << std::flush;
  auto outjar_info =
      load_jar_method_info(pg_config.basedirectory, pg_config.outjars);
  std::cout << outjar_info.size() << " method info loaded." << std::endl;

  std::cout << "Diffing in and out jars... " << std::endl;
  JarMethodInfoMap diff;
  for (auto&& pair : injar_info) {
    auto found = outjar_info.find(pair.first);
    if (found == end(outjar_info)) {
      std::cerr << "Uh-oh, " << pair.first << " can't be found in outjars"
                << std::endl;
      continue;
    }
    const auto& in = pair.second;
    const auto& out = found->second;
    if (in == out) {
      continue;
    }

    diff.emplace(pair.first,
                 std::make_tuple(std::get<0>(out) - std::get<0>(in),
                                 std::get<1>(out) - std::get<1>(in),
                                 std::get<2>(out) - std::get<2>(in)));
  }

  auto print_tuple = [](const std::tuple<int, int, int>& t) {
    return std::to_string(std::get<0>(t)) + " " +
           std::to_string(std::get<1>(t)) + " " +
           std::to_string(std::get<2>(t));
  };

  for (const auto& pair : diff) {
    std::cout << "DIFF: " << pair.first << " " << print_tuple(pair.second)
              << std::endl;
  }

  for (const auto& pair : injar_info) {
    std::cout << "IN: " << pair.first << " " << print_tuple(pair.second)
              << std::endl;
  }

  for (const auto& pair : outjar_info) {
    std::cout << "OUT: " << pair.first << " " << print_tuple(pair.second)
              << std::endl;
  }

  g_redex = injar_context;
}

using DexMethodInfoMap =
    std::unordered_map<std::string, // Method as string
                       std::tuple<int, int>>; // code size, register size

DexMethodInfoMap load_dex_method_info(const std::string& dir) {
  DexStore root_store("dex");
  DexStoresVector stores;

  // Load root dexen
  load_root_dexen(root_store, dir);
  stores.emplace_back(std::move(root_store));
  DexMethodInfoMap result;

  walk_methods(build_class_scope(stores), [&result](DexMethod* method) {
    auto key = show(method);
    always_assert(result.find(key) == end(result));
    const auto* code = method->get_dex_code();
    result.emplace(key,
                   std::make_tuple((code ? code->size() : 0),
                                   (code ? code->get_registers_size() : 0)));
  });

  return result;
}

void dump_method_sizes_from_dexen_dir(const std::string& dexen_dir) {
  std::cout << "INFO: "
            << "Loading directory " << dexen_dir << " ... " << std::endl;
  auto info = load_dex_method_info(dexen_dir);
  std::cout << "INFO: " << info.size() << " method information loaded"
            << std::endl;
  for (const auto& pair : info) {
    std::cout << "SIZE: " << pair.first << " " << std::get<0>(pair.second)
              << " " << std::get<1>(pair.second) << std::endl;
  }
}

void diff_from_two_dexen_dirs(const std::string& dexen_dir_A,
                              const std::string& dexen_dir_B) {
  std::cout << "INFO: "
            << "Loading directory " << dexen_dir_A << " ... " << std::endl;
  RedexContext* A_context = g_redex;
  auto A_info = load_dex_method_info(dexen_dir_A);
  std::cout << "INFO: " << A_info.size() << " method information loaded"
            << std::endl;

  std::cout << "INFO: "
            << "Loading directory " << dexen_dir_B << " ... " << std::endl;
  std::unique_ptr<RedexContext> B_context(new RedexContext());
  g_redex = B_context.get();
  auto B_info = load_dex_method_info(dexen_dir_B);
  std::cout << "INFO: " << B_info.size() << " method information loaded"
            << std::endl;

  std::cout << "Diffing A and B... " << std::endl;
  DexMethodInfoMap diff;
  for (auto&& pair : A_info) {
    auto found = B_info.find(pair.first);
    if (found == end(B_info)) {
      continue;
    }
    const auto& A_sizes = pair.second;
    const auto& B_sizes = found->second;
    if (A_sizes == B_sizes) {
      continue;
    }

    diff.emplace(pair.first,
                 std::make_tuple(std::get<0>(B_sizes) - std::get<0>(A_sizes),
                                 std::get<1>(B_sizes) - std::get<1>(A_sizes)));
  }

  for (const auto& pair : diff) {
    std::cout << "DIFF: " << pair.first << " " << std::get<0>(pair.second)
              << " " << std::get<1>(pair.second) << std::endl;
  }

  g_redex = A_context;
}

class DiffMethodSizes : public Tool {
 public:
  DiffMethodSizes() : Tool("diff-method-sizes", "compare method sizes") {}

  virtual void add_options(po::options_description& options) const override {
    options.add_options()(
        "commandline,c",
        po::value<std::string>(),
        "compare max_stack, max_locals, code_length of all methods in "
        "-injars and -outjars from command-line.txt")(
        "dexendir,d",
        po::value<std::vector<std::string>>()->multitoken(),
        "dump all method sizes in the given dexen directory; if two dexen "
        "directories are given, compare the method sizes");
  }

  virtual void run(const po::variables_map& options) override {
    if (!options["commandline"].empty()) {
      diff_in_out_jars_from_command_line(
          options["commandline"].as<std::string>());
    } else if (!options["dexendir"].empty()) {
      const auto& dexen_dirs =
          options["dexendir"].as<std::vector<std::string>>();
      switch (dexen_dirs.size()) {
      case 1:
        dump_method_sizes_from_dexen_dir(dexen_dirs[0]);
        break;
      case 2:
        diff_from_two_dexen_dirs(dexen_dirs[0], dexen_dirs[1]);
        break;
      default:
        std::cerr << "Only one or two --dexendir can be provided" << std::endl;
        break;
      }
    } else {
      std::cerr << "No option or invalid option was given" << std::endl;
    }
  }
};

} // namespace {

static DiffMethodSizes s_diff_method_sizes;
