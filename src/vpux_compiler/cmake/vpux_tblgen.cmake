#
# Copyright (C) 2023 Intel Corporation.
# SPDX-License-Identifier: Apache 2.0
#

add_custom_target(MLIRVPUXIncGenList)

function(add_vpux_dialect dialect_namespace)
    set(LLVM_TARGET_DEFINITIONS ops.td)
    mlir_tablegen(dialect.hpp.inc -gen-dialect-decls -dialect=${dialect_namespace})
    mlir_tablegen(dialect.cpp.inc -gen-dialect-defs -dialect=${dialect_namespace})
    mlir_tablegen(ops.hpp.inc -gen-op-decls)
    mlir_tablegen(ops.cpp.inc -gen-op-defs)
    add_public_tablegen_target(MLIRVPUX${dialect_namespace}OpsIncGen)
    add_dependencies(MLIRVPUXIncGenList MLIRVPUX${dialect_namespace}OpsIncGen)
    add_mlir_doc(ops _${dialect_namespace} dialect/ -gen-dialect-doc -dialect=${dialect_namespace})
endfunction()

function(add_vpux_ops_interface ops_namespace doc_dir)
    set(LLVM_TARGET_DEFINITIONS ops_interfaces.td)
    mlir_tablegen(ops_interfaces.hpp.inc -gen-op-interface-decls)
    mlir_tablegen(ops_interfaces.cpp.inc -gen-op-interface-defs)
    add_public_tablegen_target(MLIRVPUX${ops_namespace}OpsInterfacesIncGen)
    add_mlir_doc(ops_interfaces _${ops_namespace}_ops_interfaces ${doc_dir} -gen-op-interface-docs)
    add_dependencies(MLIRVPUXIncGenList MLIRVPUX${ops_namespace}OpsInterfacesIncGen)
endfunction()

function(add_vpux_type_interface ops_namespace doc_dir)
    set(LLVM_TARGET_DEFINITIONS type_interfaces.td)
    mlir_tablegen(type_interfaces.hpp.inc -gen-type-interface-decls)
    mlir_tablegen(type_interfaces.cpp.inc -gen-type-interface-defs)
    add_public_tablegen_target(MLIRVPUX${ops_namespace}TypeInterfacesIncGen)
    add_mlir_doc(type_interfaces _${ops_namespace}_type_interfaces ${doc_dir} -gen-type-interface-docs)
    add_dependencies(MLIRVPUXIncGenList MLIRVPUX${ops_namespace}TypeInterfacesIncGen)
endfunction()

function(add_vpux_attr_interface ops_namespace doc_dir)
    set(LLVM_TARGET_DEFINITIONS attr_interfaces.td)
    mlir_tablegen(attr_interfaces.hpp.inc -gen-attr-interface-decls)
    mlir_tablegen(attr_interfaces.cpp.inc -gen-attr-interface-defs)
    add_public_tablegen_target(MLIRVPUX${ops_namespace}AttrInterfacesIncGen)
    add_mlir_doc(attr_interfaces _${ops_namespace}_attr_interfaces ${doc_dir} -gen-attr-interface-docs)
    add_dependencies(MLIRVPUXIncGenList MLIRVPUX${ops_namespace}AttrInterfacesIncGen)
endfunction()

function(add_vpux_pass ops_namespace doc_prefix doc_dir)
    set(LLVM_TARGET_DEFINITIONS passes.td)
    mlir_tablegen(passes.hpp.inc -gen-pass-decls -name=${ops_namespace})
    add_public_tablegen_target(MLIRVPUX${doc_prefix}PassesIncGen)
    add_mlir_doc(passes _${doc_prefix}_passes ${doc_dir} -gen-pass-doc)
    add_dependencies(MLIRVPUXIncGenList MLIRVPUX${doc_prefix}PassesIncGen)
endfunction()

function(add_vpux_attribute ops_namespace)
    set(options ENABLE_VPUX_ENUMS ENABLE_VPUX_STRUCTS ENABLE_VPUX_ATTR)
    cmake_parse_arguments(ARG "${options}" "" "" ${ARGN})
    set(LLVM_TARGET_DEFINITIONS attributes.td)
    if(ARG_ENABLE_VPUX_ENUMS)
        mlir_tablegen(enums.hpp.inc -gen-enum-decls)
        mlir_tablegen(enums.cpp.inc -gen-enum-defs)
    endif()
    if(ARG_ENABLE_VPUX_STRUCTS)
        mlir_tablegen(structs.hpp.inc -gen-struct-attr-decls)
        mlir_tablegen(structs.cpp.inc -gen-struct-attr-defs)
    endif()
    if(ARG_ENABLE_VPUX_ATTR)
        mlir_tablegen(attributes.hpp.inc -gen-attrdef-decls)
        mlir_tablegen(attributes.cpp.inc -gen-attrdef-defs)
    endif()
    add_public_tablegen_target(MLIRVPUX${ops_namespace}AttrIncGen)
    add_dependencies(MLIRVPUXIncGenList MLIRVPUX${ops_namespace}AttrIncGen)
endfunction()

function(add_vpux_type ops_namespace)
    set(LLVM_TARGET_DEFINITIONS types.td)
    mlir_tablegen(types.hpp.inc -gen-typedef-decls --typedefs-dialect=${ops_namespace})
    mlir_tablegen(types.cpp.inc -gen-typedef-defs --typedefs-dialect=${ops_namespace})
    add_public_tablegen_target(MLIRVPUX${ops_namespace}TypesIncGen)
    add_dependencies(MLIRVPUXIncGenList MLIRVPUX${ops_namespace}TypesIncGen)
endfunction()

function(add_vpux_rewrite td_file ops_namespace)
    set(LLVM_TARGET_DEFINITIONS rewriters/${td_file}.td)
    mlir_tablegen(${td_file}.hpp.inc -gen-rewriters)
    add_public_tablegen_target(MLIRVPUX${ops_namespace}RewriterIncGen)
    add_dependencies(MLIRVPUXIncGenList MLIRVPUX${ops_namespace}RewriterIncGen)
endfunction()
