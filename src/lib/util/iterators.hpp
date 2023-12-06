#pragma once
#include "util/structs.hpp"

namespace util {
    template <typename Ty>
    class DerefSharedPtrIter {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::remove_const_t<Ty>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        using base_iter_type = typename std::vector<std::shared_ptr<value_type>>::iterator;
        using const_base_iter_type = typename std::vector<std::shared_ptr<value_type>>::const_iterator;
        using iter_type = std::conditional_t<std::is_const_v<Ty>, const_base_iter_type, base_iter_type>;

        explicit DerefSharedPtrIter(iter_type it): it_(it) { }
        DEFAULT_DTOR(DerefSharedPtrIter);

        DerefSharedPtrIter& operator++() {
            ++it_;
            return *this;
        }

        bool operator!=(const DerefSharedPtrIter& other) const {
            return it_ != other.it_;
        }

        Ty& operator*() const {
            return **it_;
        }

    private:
        iter_type it_;
    };
} // namespace util