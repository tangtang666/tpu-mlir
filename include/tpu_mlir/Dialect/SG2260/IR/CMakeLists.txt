set(LLVM_TARGET_DEFINITIONS SG2260Ops.td)
mlir_tablegen(SG2260Ops.h.inc -gen-op-decls)
mlir_tablegen(SG2260Ops.cpp.inc -gen-op-defs)

set(LLVM_TARGET_DEFINITIONS SG2260Types.td)
mlir_tablegen(SG2260Types.h.inc -gen-typedef-decls -typedefs-dialect=sg2260)
mlir_tablegen(SG2260Types.cpp.inc -gen-typedef-defs -typedefs-dialect=sg2260)

set(LLVM_TARGET_DEFINITIONS SG2260.td)
mlir_tablegen(SG2260Dialect.h.inc -gen-dialect-decls -dialect=sg2260)
mlir_tablegen(SG2260Dialect.cpp.inc -gen-dialect-defs -dialect=sg2260)

add_public_tablegen_target(MLIRSG2260Gen)
