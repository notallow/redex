/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "RegisterType.h"

#include "DexClass.h"
#include "DexUtil.h"
#include "IRInstruction.h"

namespace regalloc {

namespace register_type_impl {

/*
 *             UNKNOWN
 *              /    \
 *            ZERO   WIDE
 *           /    \     |
 *       OBJECT NORMAL  |
 *          \     |    /
 *           \    |   /
 *            CONFLICT
 */
Lattice lattice({RegisterType::CONFLICT,
                 RegisterType::ZERO,
                 RegisterType::NORMAL,
                 RegisterType::WIDE,
                 RegisterType::OBJECT,
                 RegisterType::UNKNOWN},
                {{RegisterType::CONFLICT, RegisterType::OBJECT},
                 {RegisterType::CONFLICT, RegisterType::NORMAL},
                 {RegisterType::CONFLICT, RegisterType::WIDE},
                 {RegisterType::OBJECT, RegisterType::ZERO},
                 {RegisterType::NORMAL, RegisterType::ZERO},
                 {RegisterType::ZERO, RegisterType::UNKNOWN},
                 {RegisterType::WIDE, RegisterType::UNKNOWN}});

} // namespace register_type_impl

std::string show(RegisterType type) {
  switch (type) {
  case RegisterType::NORMAL:
    return "NORMAL";
  case RegisterType::OBJECT:
    return "OBJECT";
  case RegisterType::WIDE:
    return "WIDE";
  case RegisterType::ZERO:
    return "ZERO";
  case RegisterType::UNKNOWN:
    return "UNKNOWN";
  case RegisterType::CONFLICT:
    return "CONFLICT";
  case RegisterType::SIZE:
    not_reached();
  }
}

static DexOpcode move_op_for_type(RegisterType type) {
  switch (type) {
  case RegisterType::ZERO:
  case RegisterType::NORMAL:
    return OPCODE_MOVE_16;
  case RegisterType::OBJECT:
    return OPCODE_MOVE_OBJECT_16;
  case RegisterType::WIDE:
    return OPCODE_MOVE_WIDE_16;
  case RegisterType::UNKNOWN:
  case RegisterType::CONFLICT:
    always_assert_log(
        false, "Cannot generate move for register type %s", SHOW(type));
  case RegisterType::SIZE:
    not_reached();
  }
}

IRInstruction* gen_move(RegisterType type, reg_t dest, reg_t src) {
  auto insn = new IRInstruction(move_op_for_type(type));
  insn->set_dest(dest);
  insn->set_src(0, src);
  return insn;
}

static RegisterType const_dest_type(const IRInstruction* insn) {
  if (insn->literal() == 0) {
    return RegisterType::ZERO;
  } else {
    return RegisterType::NORMAL;
  }
}

RegisterType dest_reg_type(const IRInstruction* insn) {
  auto op = insn->opcode();
  switch (op) {
  case OPCODE_NOP:
    always_assert_log(false, "No dest");
    not_reached();
  case OPCODE_MOVE:
    return RegisterType::NORMAL;
  case OPCODE_MOVE_WIDE:
    return RegisterType::WIDE;
  case OPCODE_MOVE_OBJECT:
    return RegisterType::OBJECT;
  case OPCODE_MOVE_RESULT:
    return RegisterType::NORMAL;
  case OPCODE_MOVE_RESULT_WIDE:
    return RegisterType::WIDE;
  case OPCODE_MOVE_RESULT_OBJECT:
  case OPCODE_MOVE_EXCEPTION:
    return RegisterType::OBJECT;
  case OPCODE_RETURN_VOID:
  case OPCODE_RETURN:
  case OPCODE_RETURN_WIDE:
  case OPCODE_RETURN_OBJECT:
    always_assert_log(false, "No dest");
    not_reached();
  case OPCODE_CONST_4:
    return const_dest_type(insn);
  case OPCODE_MONITOR_ENTER:
  case OPCODE_MONITOR_EXIT:
  case OPCODE_THROW:
  case OPCODE_GOTO:
    always_assert_log(false, "No dest");
    not_reached();
  case OPCODE_NEG_INT:
  case OPCODE_NOT_INT:
    return RegisterType::NORMAL;
  case OPCODE_NEG_LONG:
  case OPCODE_NOT_LONG:
    return RegisterType::WIDE;
  case OPCODE_NEG_FLOAT:
    return RegisterType::NORMAL;
  case OPCODE_NEG_DOUBLE:
    return RegisterType::WIDE;
  case OPCODE_INT_TO_LONG:
    return RegisterType::WIDE;
  case OPCODE_INT_TO_FLOAT:
    return RegisterType::NORMAL;
  case OPCODE_INT_TO_DOUBLE:
    return RegisterType::WIDE;
  case OPCODE_LONG_TO_INT:
  case OPCODE_LONG_TO_FLOAT:
    return RegisterType::NORMAL;
  case OPCODE_LONG_TO_DOUBLE:
    return RegisterType::WIDE;
  case OPCODE_FLOAT_TO_INT:
    return RegisterType::NORMAL;
  case OPCODE_FLOAT_TO_LONG:
  case OPCODE_FLOAT_TO_DOUBLE:
    return RegisterType::WIDE;
  case OPCODE_DOUBLE_TO_INT:
    return RegisterType::NORMAL;
  case OPCODE_DOUBLE_TO_LONG:
    return RegisterType::WIDE;
  case OPCODE_DOUBLE_TO_FLOAT:
    return RegisterType::NORMAL;
  case OPCODE_INT_TO_BYTE:
  case OPCODE_INT_TO_CHAR:
  case OPCODE_INT_TO_SHORT:
    return RegisterType::NORMAL;
  case OPCODE_ADD_INT_2ADDR:
  case OPCODE_SUB_INT_2ADDR:
  case OPCODE_MUL_INT_2ADDR:
  case OPCODE_DIV_INT_2ADDR:
  case OPCODE_REM_INT_2ADDR:
  case OPCODE_AND_INT_2ADDR:
  case OPCODE_OR_INT_2ADDR:
  case OPCODE_XOR_INT_2ADDR:
  case OPCODE_SHL_INT_2ADDR:
  case OPCODE_SHR_INT_2ADDR:
  case OPCODE_USHR_INT_2ADDR:
  case OPCODE_ADD_LONG_2ADDR:
  case OPCODE_SUB_LONG_2ADDR:
  case OPCODE_MUL_LONG_2ADDR:
  case OPCODE_DIV_LONG_2ADDR:
  case OPCODE_REM_LONG_2ADDR:
  case OPCODE_AND_LONG_2ADDR:
  case OPCODE_OR_LONG_2ADDR:
  case OPCODE_XOR_LONG_2ADDR:
  case OPCODE_SHL_LONG_2ADDR:
  case OPCODE_SHR_LONG_2ADDR:
  case OPCODE_USHR_LONG_2ADDR:
  case OPCODE_ADD_FLOAT_2ADDR:
  case OPCODE_SUB_FLOAT_2ADDR:
  case OPCODE_MUL_FLOAT_2ADDR:
  case OPCODE_DIV_FLOAT_2ADDR:
  case OPCODE_REM_FLOAT_2ADDR:
  case OPCODE_ADD_DOUBLE_2ADDR:
  case OPCODE_SUB_DOUBLE_2ADDR:
  case OPCODE_MUL_DOUBLE_2ADDR:
  case OPCODE_DIV_DOUBLE_2ADDR:
  case OPCODE_REM_DOUBLE_2ADDR:
    always_assert_log(false, "Unhandled opcode");
    not_reached();
  case OPCODE_ARRAY_LENGTH:
    return RegisterType::NORMAL;
  case OPCODE_MOVE_FROM16:
    return RegisterType::NORMAL;
  case OPCODE_MOVE_WIDE_FROM16:
    return RegisterType::WIDE;
  case OPCODE_MOVE_OBJECT_FROM16:
    return RegisterType::OBJECT;
  case OPCODE_CONST_16:
  case OPCODE_CONST_HIGH16:
    return const_dest_type(insn);
  case OPCODE_CONST_WIDE_16:
  case OPCODE_CONST_WIDE_HIGH16:
    return RegisterType::WIDE;
  case OPCODE_GOTO_16:
    always_assert_log(false, "No dest");
    not_reached();
  case OPCODE_CMPL_FLOAT:
  case OPCODE_CMPG_FLOAT:
  case OPCODE_CMPL_DOUBLE:
  case OPCODE_CMPG_DOUBLE:
  case OPCODE_CMP_LONG:
    return RegisterType::NORMAL;
  case OPCODE_IF_EQ:
  case OPCODE_IF_NE:
  case OPCODE_IF_LT:
  case OPCODE_IF_GE:
  case OPCODE_IF_GT:
  case OPCODE_IF_LE:
  case OPCODE_IF_EQZ:
  case OPCODE_IF_NEZ:
  case OPCODE_IF_LTZ:
  case OPCODE_IF_GEZ:
  case OPCODE_IF_GTZ:
  case OPCODE_IF_LEZ:
    always_assert_log(false, "No dest");
    not_reached();
  case OPCODE_AGET:
    return RegisterType::NORMAL;
  case OPCODE_AGET_WIDE:
    return RegisterType::WIDE;
  case OPCODE_AGET_OBJECT:
    return RegisterType::OBJECT;
  case OPCODE_AGET_BOOLEAN:
  case OPCODE_AGET_BYTE:
  case OPCODE_AGET_CHAR:
  case OPCODE_AGET_SHORT:
    return RegisterType::NORMAL;
  case OPCODE_APUT:
  case OPCODE_APUT_WIDE:
  case OPCODE_APUT_OBJECT:
  case OPCODE_APUT_BOOLEAN:
  case OPCODE_APUT_BYTE:
  case OPCODE_APUT_CHAR:
  case OPCODE_APUT_SHORT:
    always_assert_log(false, "No dest");
    not_reached();
  case OPCODE_ADD_INT:
  case OPCODE_SUB_INT:
  case OPCODE_MUL_INT:
  case OPCODE_DIV_INT:
  case OPCODE_REM_INT:
  case OPCODE_AND_INT:
  case OPCODE_OR_INT:
  case OPCODE_XOR_INT:
  case OPCODE_SHL_INT:
  case OPCODE_SHR_INT:
  case OPCODE_USHR_INT:
    return RegisterType::NORMAL;
  case OPCODE_ADD_LONG:
  case OPCODE_SUB_LONG:
  case OPCODE_MUL_LONG:
  case OPCODE_DIV_LONG:
  case OPCODE_REM_LONG:
  case OPCODE_AND_LONG:
  case OPCODE_OR_LONG:
  case OPCODE_XOR_LONG:
  case OPCODE_SHL_LONG:
  case OPCODE_SHR_LONG:
  case OPCODE_USHR_LONG:
    return RegisterType::WIDE;
  case OPCODE_ADD_FLOAT:
  case OPCODE_SUB_FLOAT:
  case OPCODE_MUL_FLOAT:
  case OPCODE_DIV_FLOAT:
  case OPCODE_REM_FLOAT:
    return RegisterType::NORMAL;
  case OPCODE_ADD_DOUBLE:
  case OPCODE_SUB_DOUBLE:
  case OPCODE_MUL_DOUBLE:
  case OPCODE_DIV_DOUBLE:
  case OPCODE_REM_DOUBLE:
    return RegisterType::WIDE;
  case OPCODE_ADD_INT_LIT16:
  case OPCODE_RSUB_INT:
  case OPCODE_MUL_INT_LIT16:
  case OPCODE_DIV_INT_LIT16:
  case OPCODE_REM_INT_LIT16:
  case OPCODE_AND_INT_LIT16:
  case OPCODE_OR_INT_LIT16:
  case OPCODE_XOR_INT_LIT16:
  case OPCODE_ADD_INT_LIT8:
  case OPCODE_RSUB_INT_LIT8:
  case OPCODE_MUL_INT_LIT8:
  case OPCODE_DIV_INT_LIT8:
  case OPCODE_REM_INT_LIT8:
  case OPCODE_AND_INT_LIT8:
  case OPCODE_OR_INT_LIT8:
  case OPCODE_XOR_INT_LIT8:
  case OPCODE_SHL_INT_LIT8:
  case OPCODE_SHR_INT_LIT8:
  case OPCODE_USHR_INT_LIT8:
    return RegisterType::NORMAL;
  case OPCODE_MOVE_16:
    return RegisterType::NORMAL;
  case OPCODE_MOVE_WIDE_16:
    return RegisterType::WIDE;
  case OPCODE_MOVE_OBJECT_16:
    return RegisterType::OBJECT;
  case OPCODE_CONST:
    return const_dest_type(insn);
  case OPCODE_CONST_WIDE_32:
    return RegisterType::WIDE;
  case OPCODE_FILL_ARRAY_DATA:
  case OPCODE_GOTO_32:
  case OPCODE_PACKED_SWITCH:
  case OPCODE_SPARSE_SWITCH:
    always_assert_log(false, "No dest");
    not_reached();
  case OPCODE_CONST_WIDE:
    return RegisterType::WIDE;
  case OPCODE_IGET:
    return RegisterType::NORMAL;
  case OPCODE_IGET_WIDE:
    return RegisterType::WIDE;
  case OPCODE_IGET_OBJECT:
    return RegisterType::OBJECT;
  case OPCODE_IGET_BOOLEAN:
  case OPCODE_IGET_BYTE:
  case OPCODE_IGET_CHAR:
  case OPCODE_IGET_SHORT:
    return RegisterType::NORMAL;
  case OPCODE_IPUT:
  case OPCODE_IPUT_WIDE:
  case OPCODE_IPUT_OBJECT:
  case OPCODE_IPUT_BOOLEAN:
  case OPCODE_IPUT_BYTE:
  case OPCODE_IPUT_CHAR:
  case OPCODE_IPUT_SHORT:
    always_assert_log(false, "No dest");
    not_reached();
  case OPCODE_SGET:
    return RegisterType::NORMAL;
  case OPCODE_SGET_WIDE:
    return RegisterType::WIDE;
  case OPCODE_SGET_OBJECT:
    return RegisterType::OBJECT;
  case OPCODE_SGET_BOOLEAN:
  case OPCODE_SGET_BYTE:
  case OPCODE_SGET_CHAR:
  case OPCODE_SGET_SHORT:
    return RegisterType::NORMAL;
  case OPCODE_SPUT:
  case OPCODE_SPUT_WIDE:
  case OPCODE_SPUT_OBJECT:
  case OPCODE_SPUT_BOOLEAN:
  case OPCODE_SPUT_BYTE:
  case OPCODE_SPUT_CHAR:
  case OPCODE_SPUT_SHORT:
    always_assert_log(false, "No dest");
    not_reached();
  case OPCODE_INVOKE_VIRTUAL:
  case OPCODE_INVOKE_SUPER:
  case OPCODE_INVOKE_DIRECT:
  case OPCODE_INVOKE_STATIC:
  case OPCODE_INVOKE_INTERFACE:
  case OPCODE_INVOKE_VIRTUAL_RANGE:
  case OPCODE_INVOKE_SUPER_RANGE:
  case OPCODE_INVOKE_DIRECT_RANGE:
  case OPCODE_INVOKE_STATIC_RANGE:
  case OPCODE_INVOKE_INTERFACE_RANGE:
    always_assert_log(false, "No dest");
    not_reached();
  case OPCODE_CONST_STRING:
  case OPCODE_CONST_STRING_JUMBO:
  case OPCODE_CONST_CLASS:
  case OPCODE_CHECK_CAST:
    return RegisterType::OBJECT;
  case OPCODE_INSTANCE_OF:
    return RegisterType::NORMAL;
  case OPCODE_NEW_INSTANCE:
  case OPCODE_NEW_ARRAY:
  case OPCODE_FILLED_NEW_ARRAY:
  case OPCODE_FILLED_NEW_ARRAY_RANGE:
    return RegisterType::OBJECT;
  case IOPCODE_LOAD_PARAM:
    return RegisterType::NORMAL;
  case IOPCODE_LOAD_PARAM_OBJECT:
    return RegisterType::OBJECT;
  case IOPCODE_LOAD_PARAM_WIDE:
    return RegisterType::WIDE;
  default:
    always_assert_log(false, "Unknown opcode %02x\n", op);
  }
}

static RegisterType invoke_src_type(const IRInstruction* insn, reg_t i) {
  auto* method = insn->get_method();
  // non-static invokes have an implicit `this` arg that is not reflected in
  // the method proto.
  if (insn->opcode() != OPCODE_INVOKE_STATIC) {
    if (i == 0) {
      return RegisterType::OBJECT;
    } else {
      // decrement `i` by one so that we can use it as an index into the
      // argument type list.
      --i;
    }
  }
  const auto& types = method->get_proto()->get_args()->get_type_list();
  auto* type = types.at(i);
  if (is_wide_type(type)) {
    return RegisterType::WIDE;
  } else if (is_primitive(types.at(i))) {
    return RegisterType::NORMAL;
  } else {
    return RegisterType::OBJECT;
  }
}

RegisterType src_reg_type(const IRInstruction* insn, reg_t i) {
  auto op = insn->opcode();
  switch (op) {
  case OPCODE_NOP:
    always_assert_log(false, "No src");
    not_reached();
  case OPCODE_MOVE:
    return RegisterType::NORMAL;
  case OPCODE_MOVE_WIDE:
    return RegisterType::WIDE;
  case OPCODE_MOVE_OBJECT:
    return RegisterType::OBJECT;
  case OPCODE_MOVE_RESULT:
  case OPCODE_MOVE_RESULT_WIDE:
  case OPCODE_MOVE_RESULT_OBJECT:
  case OPCODE_MOVE_EXCEPTION:
  case OPCODE_RETURN_VOID:
    always_assert_log(false, "No src");
    not_reached();
  case OPCODE_RETURN:
    return RegisterType::NORMAL;
  case OPCODE_RETURN_WIDE:
    return RegisterType::WIDE;
  case OPCODE_RETURN_OBJECT:
    return RegisterType::OBJECT;
  case OPCODE_CONST_4:
    always_assert_log(false, "No src");
    not_reached();
  case OPCODE_MONITOR_ENTER:
  case OPCODE_MONITOR_EXIT:
  case OPCODE_THROW:
    return RegisterType::OBJECT;
  case OPCODE_GOTO:
    always_assert_log(false, "No src");
    not_reached();
  case OPCODE_NEG_INT:
  case OPCODE_NOT_INT:
    return RegisterType::NORMAL;
  case OPCODE_NEG_LONG:
  case OPCODE_NOT_LONG:
    return RegisterType::WIDE;
  case OPCODE_NEG_FLOAT:
    return RegisterType::NORMAL;
  case OPCODE_NEG_DOUBLE:
    return RegisterType::WIDE;
  case OPCODE_INT_TO_LONG:
  case OPCODE_INT_TO_FLOAT:
  case OPCODE_INT_TO_DOUBLE:
    return RegisterType::NORMAL;
  case OPCODE_LONG_TO_INT:
  case OPCODE_LONG_TO_FLOAT:
  case OPCODE_LONG_TO_DOUBLE:
    return RegisterType::WIDE;
  case OPCODE_FLOAT_TO_INT:
  case OPCODE_FLOAT_TO_LONG:
  case OPCODE_FLOAT_TO_DOUBLE:
    return RegisterType::NORMAL;
  case OPCODE_DOUBLE_TO_INT:
  case OPCODE_DOUBLE_TO_LONG:
  case OPCODE_DOUBLE_TO_FLOAT:
    return RegisterType::WIDE;
  case OPCODE_INT_TO_BYTE:
  case OPCODE_INT_TO_CHAR:
  case OPCODE_INT_TO_SHORT:
    return RegisterType::NORMAL;
  case OPCODE_ADD_INT_2ADDR:
  case OPCODE_SUB_INT_2ADDR:
  case OPCODE_MUL_INT_2ADDR:
  case OPCODE_DIV_INT_2ADDR:
  case OPCODE_REM_INT_2ADDR:
  case OPCODE_AND_INT_2ADDR:
  case OPCODE_OR_INT_2ADDR:
  case OPCODE_XOR_INT_2ADDR:
  case OPCODE_SHL_INT_2ADDR:
  case OPCODE_SHR_INT_2ADDR:
  case OPCODE_USHR_INT_2ADDR:
  case OPCODE_ADD_LONG_2ADDR:
  case OPCODE_SUB_LONG_2ADDR:
  case OPCODE_MUL_LONG_2ADDR:
  case OPCODE_DIV_LONG_2ADDR:
  case OPCODE_REM_LONG_2ADDR:
  case OPCODE_AND_LONG_2ADDR:
  case OPCODE_OR_LONG_2ADDR:
  case OPCODE_XOR_LONG_2ADDR:
  case OPCODE_SHL_LONG_2ADDR:
  case OPCODE_SHR_LONG_2ADDR:
  case OPCODE_USHR_LONG_2ADDR:
  case OPCODE_ADD_FLOAT_2ADDR:
  case OPCODE_SUB_FLOAT_2ADDR:
  case OPCODE_MUL_FLOAT_2ADDR:
  case OPCODE_DIV_FLOAT_2ADDR:
  case OPCODE_REM_FLOAT_2ADDR:
  case OPCODE_ADD_DOUBLE_2ADDR:
  case OPCODE_SUB_DOUBLE_2ADDR:
  case OPCODE_MUL_DOUBLE_2ADDR:
  case OPCODE_DIV_DOUBLE_2ADDR:
  case OPCODE_REM_DOUBLE_2ADDR:
    always_assert_log(false, "Unhandled opcode");
    not_reached();
  case OPCODE_ARRAY_LENGTH:
    return RegisterType::OBJECT;
  case OPCODE_MOVE_FROM16:
    return RegisterType::NORMAL;
  case OPCODE_MOVE_WIDE_FROM16:
    return RegisterType::WIDE;
  case OPCODE_MOVE_OBJECT_FROM16:
    return RegisterType::OBJECT;
  case OPCODE_CONST_16:
  case OPCODE_CONST_HIGH16:
  case OPCODE_CONST_WIDE_16:
  case OPCODE_CONST_WIDE_HIGH16:
  case OPCODE_GOTO_16:
    always_assert_log(false, "No src");
    not_reached();
  case OPCODE_CMPL_FLOAT:
  case OPCODE_CMPG_FLOAT:
    return RegisterType::NORMAL;
  case OPCODE_CMPL_DOUBLE:
  case OPCODE_CMPG_DOUBLE:
  case OPCODE_CMP_LONG:
    return RegisterType::WIDE;
  case OPCODE_IF_EQ:
  case OPCODE_IF_NE:
  case OPCODE_IF_LT:
  case OPCODE_IF_GE:
  case OPCODE_IF_GT:
  case OPCODE_IF_LE:
  case OPCODE_IF_EQZ:
  case OPCODE_IF_NEZ:
  case OPCODE_IF_LTZ:
  case OPCODE_IF_GEZ:
  case OPCODE_IF_GTZ:
  case OPCODE_IF_LEZ:
    // can either be primitive or ref
    return RegisterType::UNKNOWN;
  case OPCODE_AGET:
  case OPCODE_AGET_WIDE:
  case OPCODE_AGET_OBJECT:
  case OPCODE_AGET_BOOLEAN:
  case OPCODE_AGET_BYTE:
  case OPCODE_AGET_CHAR:
  case OPCODE_AGET_SHORT:
    return i == 0 ? RegisterType::OBJECT : RegisterType::NORMAL;
  case OPCODE_APUT:
    return i == 1 ? RegisterType::OBJECT : RegisterType::NORMAL;
  case OPCODE_APUT_WIDE:
    return i == 1 ? RegisterType::OBJECT
                  : i == 2 ? RegisterType::NORMAL : RegisterType::WIDE;
  case OPCODE_APUT_OBJECT:
    return i <= 1 ? RegisterType::OBJECT : RegisterType::NORMAL;
  case OPCODE_APUT_BOOLEAN:
  case OPCODE_APUT_BYTE:
  case OPCODE_APUT_CHAR:
  case OPCODE_APUT_SHORT:
    return i == 1 ? RegisterType::OBJECT : RegisterType::NORMAL;
  case OPCODE_ADD_INT:
  case OPCODE_SUB_INT:
  case OPCODE_MUL_INT:
  case OPCODE_DIV_INT:
  case OPCODE_REM_INT:
  case OPCODE_AND_INT:
  case OPCODE_OR_INT:
  case OPCODE_XOR_INT:
  case OPCODE_SHL_INT:
  case OPCODE_SHR_INT:
  case OPCODE_USHR_INT:
    return RegisterType::NORMAL;
  case OPCODE_ADD_LONG:
  case OPCODE_SUB_LONG:
  case OPCODE_MUL_LONG:
  case OPCODE_DIV_LONG:
  case OPCODE_REM_LONG:
  case OPCODE_AND_LONG:
  case OPCODE_OR_LONG:
  case OPCODE_XOR_LONG:
    return RegisterType::WIDE;
  case OPCODE_SHL_LONG:
  case OPCODE_SHR_LONG:
  case OPCODE_USHR_LONG:
    return i == 0 ? RegisterType::WIDE : RegisterType::NORMAL;
  case OPCODE_ADD_FLOAT:
  case OPCODE_SUB_FLOAT:
  case OPCODE_MUL_FLOAT:
  case OPCODE_DIV_FLOAT:
  case OPCODE_REM_FLOAT:
    return RegisterType::NORMAL;
  case OPCODE_ADD_DOUBLE:
  case OPCODE_SUB_DOUBLE:
  case OPCODE_MUL_DOUBLE:
  case OPCODE_DIV_DOUBLE:
  case OPCODE_REM_DOUBLE:
    return RegisterType::WIDE;
  case OPCODE_ADD_INT_LIT16:
  case OPCODE_RSUB_INT:
  case OPCODE_MUL_INT_LIT16:
  case OPCODE_DIV_INT_LIT16:
  case OPCODE_REM_INT_LIT16:
  case OPCODE_AND_INT_LIT16:
  case OPCODE_OR_INT_LIT16:
  case OPCODE_XOR_INT_LIT16:
  case OPCODE_ADD_INT_LIT8:
  case OPCODE_RSUB_INT_LIT8:
  case OPCODE_MUL_INT_LIT8:
  case OPCODE_DIV_INT_LIT8:
  case OPCODE_REM_INT_LIT8:
  case OPCODE_AND_INT_LIT8:
  case OPCODE_OR_INT_LIT8:
  case OPCODE_XOR_INT_LIT8:
  case OPCODE_SHL_INT_LIT8:
  case OPCODE_SHR_INT_LIT8:
  case OPCODE_USHR_INT_LIT8:
    return RegisterType::NORMAL;
  case OPCODE_MOVE_16:
    return RegisterType::NORMAL;
  case OPCODE_MOVE_WIDE_16:
    return RegisterType::WIDE;
  case OPCODE_MOVE_OBJECT_16:
    return RegisterType::OBJECT;
  case OPCODE_CONST:
  case OPCODE_CONST_WIDE_32:
    always_assert_log(false, "No src");
    not_reached();
  case OPCODE_FILL_ARRAY_DATA:
    return RegisterType::OBJECT;
  case OPCODE_GOTO_32:
    always_assert_log(false, "No src");
    not_reached();
  case OPCODE_PACKED_SWITCH:
  case OPCODE_SPARSE_SWITCH:
    return RegisterType::UNKNOWN;
  case OPCODE_CONST_WIDE:
    always_assert_log(false, "No src");
    not_reached();
  case OPCODE_IGET:
  case OPCODE_IGET_WIDE:
  case OPCODE_IGET_OBJECT:
  case OPCODE_IGET_BOOLEAN:
  case OPCODE_IGET_BYTE:
  case OPCODE_IGET_CHAR:
  case OPCODE_IGET_SHORT:
    always_assert(i == 0);
    return RegisterType::OBJECT;
  case OPCODE_IPUT:
    return i == 1 ? RegisterType::OBJECT : RegisterType::NORMAL;
  case OPCODE_IPUT_WIDE:
    return i == 1 ? RegisterType::OBJECT : RegisterType::WIDE;
  case OPCODE_IPUT_OBJECT:
    return RegisterType::OBJECT;
  case OPCODE_IPUT_BOOLEAN:
  case OPCODE_IPUT_BYTE:
  case OPCODE_IPUT_CHAR:
  case OPCODE_IPUT_SHORT:
    return i == 1 ? RegisterType::OBJECT : RegisterType::NORMAL;
  case OPCODE_SGET:
  case OPCODE_SGET_WIDE:
  case OPCODE_SGET_OBJECT:
  case OPCODE_SGET_BOOLEAN:
  case OPCODE_SGET_BYTE:
  case OPCODE_SGET_CHAR:
  case OPCODE_SGET_SHORT:
    always_assert_log(false, "No src");
    not_reached();
  case OPCODE_SPUT:
    return RegisterType::NORMAL;
  case OPCODE_SPUT_WIDE:
    return RegisterType::WIDE;
  case OPCODE_SPUT_OBJECT:
    return RegisterType::OBJECT;
  case OPCODE_SPUT_BOOLEAN:
  case OPCODE_SPUT_BYTE:
  case OPCODE_SPUT_CHAR:
  case OPCODE_SPUT_SHORT:
    return RegisterType::NORMAL;
  case OPCODE_INVOKE_VIRTUAL:
  case OPCODE_INVOKE_SUPER:
  case OPCODE_INVOKE_DIRECT:
  case OPCODE_INVOKE_STATIC:
  case OPCODE_INVOKE_INTERFACE:
    return invoke_src_type(insn, i);
  case OPCODE_INVOKE_VIRTUAL_RANGE:
  case OPCODE_INVOKE_SUPER_RANGE:
  case OPCODE_INVOKE_DIRECT_RANGE:
  case OPCODE_INVOKE_STATIC_RANGE:
  case OPCODE_INVOKE_INTERFACE_RANGE:
    always_assert_log(false, "Unhandled opcode");
    not_reached();
  case OPCODE_CONST_STRING:
  case OPCODE_CONST_STRING_JUMBO:
  case OPCODE_CONST_CLASS:
    always_assert_log(false, "No src");
    not_reached();
  case OPCODE_CHECK_CAST:
  case OPCODE_INSTANCE_OF:
    return RegisterType::OBJECT;
  case OPCODE_NEW_INSTANCE:
    always_assert_log(false, "No src");
    not_reached();
  case OPCODE_NEW_ARRAY:
    return RegisterType::NORMAL;
  case OPCODE_FILLED_NEW_ARRAY:
    return is_primitive(get_array_type(insn->get_type()))
               ? RegisterType::NORMAL
               : RegisterType::OBJECT;
  case OPCODE_FILLED_NEW_ARRAY_RANGE:
    always_assert_log(false, "Unhandled opcode");
    not_reached();
  case IOPCODE_LOAD_PARAM:
  case IOPCODE_LOAD_PARAM_OBJECT:
  case IOPCODE_LOAD_PARAM_WIDE:
    always_assert_log(false, "No src");
    not_reached();
  default:
    always_assert_log(false, "Unknown opcode %02x\n", op);
  }
}

} // namespace regalloc
