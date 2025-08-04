#include "core_functions/scalar/generic_functions.hpp"
#include "duckdb/common/types/hash.hpp"

namespace duckdb {

static void HashFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	args.Hash(result);
	if (args.AllConstant()) {
		result.SetVectorType(VectorType::CONSTANT_VECTOR);
	}
}

static void HashLegacyFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	// This is a simplified version that hashes each column separately and combines them
	// For full compatibility, we would need to reimplement the entire vectorized hash operation
	D_ASSERT(args.ColumnCount() >= 1);
	
	auto count = args.size();
	result.SetVectorType(VectorType::FLAT_VECTOR);
	
	// Initialize result with hash of first column
	auto first_col = args.data[0];
	first_col.Flatten(count);
	
	auto result_data = FlatVector::GetData<hash_t>(result);
	
	// Handle the first column
	switch (first_col.GetType().id()) {
	case LogicalTypeId::BIGINT: {
		auto data = FlatVector::GetData<int64_t>(first_col);
		auto &mask = FlatVector::Validity(first_col);
		for (idx_t i = 0; i < count; i++) {
			if (!mask.RowIsValid(i)) {
				result_data[i] = HashLegacy<uint64_t>(13787848793156543929ULL); // NULL hash value
			} else {
				result_data[i] = HashLegacy<int64_t>(data[i]);
			}
		}
		break;
	}
	case LogicalTypeId::INTEGER: {
		auto data = FlatVector::GetData<int32_t>(first_col);
		auto &mask = FlatVector::Validity(first_col);
		for (idx_t i = 0; i < count; i++) {
			if (!mask.RowIsValid(i)) {
				result_data[i] = HashLegacy<uint64_t>(13787848793156543929ULL);
			} else {
				result_data[i] = HashLegacy<int64_t>(data[i]);
			}
		}
		break;
	}
	case LogicalTypeId::VARCHAR: {
		auto data = FlatVector::GetData<string_t>(first_col);
		auto &mask = FlatVector::Validity(first_col);
		for (idx_t i = 0; i < count; i++) {
			if (!mask.RowIsValid(i)) {
				result_data[i] = HashLegacy<uint64_t>(13787848793156543929ULL);
			} else {
				result_data[i] = HashLegacy<string_t>(data[i]);
			}
		}
		break;
	}
	case LogicalTypeId::DOUBLE: {
		auto data = FlatVector::GetData<double>(first_col);
		auto &mask = FlatVector::Validity(first_col);
		for (idx_t i = 0; i < count; i++) {
			if (!mask.RowIsValid(i)) {
				result_data[i] = HashLegacy<uint64_t>(13787848793156543929ULL);
			} else {
				result_data[i] = HashLegacy<double>(data[i]);
			}
		}
		break;
	}
	case LogicalTypeId::FLOAT: {
		auto data = FlatVector::GetData<float>(first_col);
		auto &mask = FlatVector::Validity(first_col);
		for (idx_t i = 0; i < count; i++) {
			if (!mask.RowIsValid(i)) {
				result_data[i] = HashLegacy<uint64_t>(13787848793156543929ULL);
			} else {
				result_data[i] = HashLegacy<float>(data[i]);
			}
		}
		break;
	}
	default: {
		// For other types, fall back to string representation
		UnifiedVectorFormat format;
		first_col.ToUnifiedFormat(count, format);
		for (idx_t i = 0; i < count; i++) {
			auto idx = format.sel->get_index(i);
			if (!format.validity.RowIsValid(idx)) {
				result_data[i] = HashLegacy<uint64_t>(13787848793156543929ULL);
			} else {
				auto str_val = first_col.GetValue(i).ToString();
				result_data[i] = HashBytesLegacy(const_data_ptr_cast(str_val.c_str()), str_val.length());
			}
		}
		break;
		}
	}
	
	// For additional columns, combine their hashes
	for (idx_t col_idx = 1; col_idx < args.ColumnCount(); col_idx++) {
		auto &col = args.data[col_idx];
		col.Flatten(count);
		
		switch (col.GetType().id()) {
		case LogicalTypeId::BIGINT: {
			auto data = FlatVector::GetData<int64_t>(col);
			auto &mask = FlatVector::Validity(col);
			for (idx_t i = 0; i < count; i++) {
				hash_t col_hash;
				if (!mask.RowIsValid(i)) {
					col_hash = HashLegacy<uint64_t>(13787848793156543929ULL);
				} else {
					col_hash = HashLegacy<int64_t>(data[i]);
				}
				result_data[i] = CombineHash(result_data[i], col_hash);
			}
			break;
		}
		case LogicalTypeId::INTEGER: {
			auto data = FlatVector::GetData<int32_t>(col);
			auto &mask = FlatVector::Validity(col);
			for (idx_t i = 0; i < count; i++) {
				hash_t col_hash;
				if (!mask.RowIsValid(i)) {
					col_hash = HashLegacy<uint64_t>(13787848793156543929ULL);
				} else {
					col_hash = HashLegacy<int64_t>(data[i]);
				}
				result_data[i] = CombineHash(result_data[i], col_hash);
			}
			break;
		}
		case LogicalTypeId::VARCHAR: {
			auto data = FlatVector::GetData<string_t>(col);
			auto &mask = FlatVector::Validity(col);
			for (idx_t i = 0; i < count; i++) {
				hash_t col_hash;
				if (!mask.RowIsValid(i)) {
					col_hash = HashLegacy<uint64_t>(13787848793156543929ULL);
				} else {
					col_hash = HashLegacy<string_t>(data[i]);
				}
				result_data[i] = CombineHash(result_data[i], col_hash);
			}
			break;
		}
		case LogicalTypeId::DOUBLE: {
			auto data = FlatVector::GetData<double>(col);
			auto &mask = FlatVector::Validity(col);
			for (idx_t i = 0; i < count; i++) {
				hash_t col_hash;
				if (!mask.RowIsValid(i)) {
					col_hash = HashLegacy<uint64_t>(13787848793156543929ULL);
				} else {
					col_hash = HashLegacy<double>(data[i]);
				}
				result_data[i] = CombineHash(result_data[i], col_hash);
			}
			break;
		}
		case LogicalTypeId::FLOAT: {
			auto data = FlatVector::GetData<float>(col);
			auto &mask = FlatVector::Validity(col);
			for (idx_t i = 0; i < count; i++) {
				hash_t col_hash;
				if (!mask.RowIsValid(i)) {
					col_hash = HashLegacy<uint64_t>(13787848793156543929ULL);
				} else {
					col_hash = HashLegacy<float>(data[i]);
				}
				result_data[i] = CombineHash(result_data[i], col_hash);
			}
			break;
		}
		default: {
			// For other types, fall back to string representation
			UnifiedVectorFormat col_format;
			col.ToUnifiedFormat(count, col_format);
			for (idx_t i = 0; i < count; i++) {
				hash_t col_hash;
				auto idx = col_format.sel->get_index(i);
				if (!col_format.validity.RowIsValid(idx)) {
					col_hash = HashLegacy<uint64_t>(13787848793156543929ULL);
				} else {
					auto str_val = col.GetValue(i).ToString();
					col_hash = HashBytesLegacy(const_data_ptr_cast(str_val.c_str()), str_val.length());
				}
				result_data[i] = CombineHash(result_data[i], col_hash);
			}
			break;
			}
		}
	}
	
	if (args.AllConstant()) {
		result.SetVectorType(VectorType::CONSTANT_VECTOR);
	}
}

ScalarFunction HashFun::GetFunction() {
	auto hash_fun = ScalarFunction({LogicalType::ANY}, LogicalType::HASH, HashFunction);
	hash_fun.varargs = LogicalType::ANY;
	hash_fun.null_handling = FunctionNullHandling::SPECIAL_HANDLING;
	return hash_fun;
}

ScalarFunction HashLegacyFun::GetFunction() {
	auto hash_legacy_fun = ScalarFunction({LogicalType::ANY}, LogicalType::HASH, HashLegacyFunction);
	hash_legacy_fun.varargs = LogicalType::ANY;
	hash_legacy_fun.null_handling = FunctionNullHandling::SPECIAL_HANDLING;
	return hash_legacy_fun;
}

} // namespace duckdb
