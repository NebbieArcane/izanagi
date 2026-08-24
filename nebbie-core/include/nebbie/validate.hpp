#pragma once

#include "world.hpp"

#include <string>
#include <vector>

namespace nebbie {

enum class ValidationSeverity {
    warning,
    error,
};

enum class ValidationTarget {
    none,
    room,
    mob,
    object,
    zone,
    shop,
};

struct ValidationIssue {
    ValidationSeverity severity = ValidationSeverity::error;
    std::string category;
    std::string message;
    ValidationTarget target = ValidationTarget::none;
    long target_vnum = 0;
    int zone_num = 0;
    int reset_index = -1;
};

struct ValidationReport {
    std::vector<ValidationIssue> issues;

    bool ok() const;
    std::size_t error_count() const;
    std::size_t warning_count() const;
};

struct ValidationOptions {
    int max_line_length = 0;
};

ValidationReport validate_world(const World& world, const ValidationOptions& options = {});
ValidationReport validate_rooms(const World& world,
                                const ValidationOptions& options = {},
                                const std::vector<long>* room_vnums = nullptr);
ValidationReport validate_translatable_rooms(const World& world,
                                             const ValidationOptions& options = {},
                                             const std::vector<long>* room_vnums = nullptr);

} // namespace nebbie
