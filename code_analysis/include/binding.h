#ifndef BINDING_H
#define BINDING_H

#include <memory>
#include <string>
#include "types.h"

struct Binding {
    int scope_depth;
    int slot_index;
    std::shared_ptr<Type> type;
};

#endif
