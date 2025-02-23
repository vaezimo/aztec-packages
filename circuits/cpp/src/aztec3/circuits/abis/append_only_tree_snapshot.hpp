#pragma once

#include <barretenberg/barretenberg.hpp>

namespace aztec3::circuits::abis {

template <typename NCT> struct AppendOnlyTreeSnapshot {
    using fr = typename NCT::fr;
    using uint32 = typename NCT::uint32;

    fr root = 0;
    uint32 next_available_leaf_index;
    MSGPACK_FIELDS(root, next_available_leaf_index);

    bool operator==(AppendOnlyTreeSnapshot<NCT> const&) const = default;
};

template <typename NCT> void read(uint8_t const*& it, AppendOnlyTreeSnapshot<NCT>& obj)
{
    using serialize::read;

    read(it, obj.root);
    read(it, obj.next_available_leaf_index);
};

template <typename NCT> void write(std::vector<uint8_t>& buf, AppendOnlyTreeSnapshot<NCT> const& obj)
{
    using serialize::write;

    write(buf, obj.root);
    write(buf, obj.next_available_leaf_index);
};

template <typename NCT> std::ostream& operator<<(std::ostream& os, AppendOnlyTreeSnapshot<NCT> const& obj)
{
    return os << "root: " << obj.root << "\n"
              << "next_available_leaf_index: " << obj.next_available_leaf_index << "\n";
}

}  // namespace aztec3::circuits::abis