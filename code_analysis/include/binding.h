#ifndef BINDING_H
#define BINDING_H

#include <memory>
#include <string>
#include "types.h"

struct Binding {
    int scope_depth = -1;
    int slot_index = -1;
    std::shared_ptr<Type> type;
};

#endif
