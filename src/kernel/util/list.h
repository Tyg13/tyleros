#ifndef UTIL_LIST_H
#define UTIL_LIST_H

#include "allocator.h"

namespace kstd {
   template <typename T, typename Allocator = kstd::allocator<T>>
   class list {
      struct node {
         T      data;
         node * next = nullptr;
      };
      struct iterator {
         explicit iterator(const node * const & _inner) : inner(_inner) {}

         iterator & operator++()       { inner = inner->next; return *this; }
         T        & operator* () const { return inner->data; }

         friend bool operator!=(const iterator& lhs, const iterator& rhs) { return lhs.inner != rhs.inner; }

         // Comparison with end iterator is always false,
         // unless we have two end iterators
         struct end {
            friend bool operator!=(const end&, const end&)      { return true;  }
            friend bool operator!=(const end&, const iterator&) { return false; }
            friend bool operator!=(const iterator&, const end&) { return false; }
         };
      private:
         const node * const & inner = nullptr;
      };
      using end_iterator   = typename iterator::end;
   public:
      using iterator       = struct iterator;
      using const_iterator = const iterator;
      using allocator_type = Allocator;
      using node_allocator = typename allocator_type::template rebind<node>;

      list() {}
      ~list() {
         auto curr = head;
         while (curr) {
            auto next = curr->next;
            node_allocator().deallocate(curr);
            curr = next;
         }
      }

      void push_back(const T & value) {
         node ** where_to_place_new_node;
         if (head == nullptr) {
            where_to_place_new_node = &head;
         } else {
            while (head->next) {
               head = head->next;
            }
            where_to_place_new_node = &head->next;
         }
         auto new_node = node_allocator().allocate(sizeof(node));
         *new_node = node { .data = value, .next = nullptr };
         *where_to_place_new_node = new_node;
      }

      iterator       begin()       { return iterator      (head); }
      const_iterator begin() const { return const_iterator(head); }

      end_iterator end()       { return end_iterator{}; }
      end_iterator end() const { return end_iterator{}; }

      allocator_type get_allocator() const { return allocator_type(); }

   private:
      node * head = nullptr;
   };
}

#endif
