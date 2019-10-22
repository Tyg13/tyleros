#ifndef UTIL_LIST_H
#define UTIL_LIST_H

#include "allocator.h"

namespace kstd {
   template <typename T, typename Allocator = kstd::allocator<T>>
   class list {
      struct node {
         T    data;
         node * next = nullptr;
      };
      struct iterator {
         iterator(node * _inner) : inner(_inner) {}

         iterator& operator++()       { inner = inner->next; return *this; }
         T &       operator* () const { return inner->data; }

         friend bool operator!=(const iterator& lhs, const iterator& rhs) { return lhs.inner != rhs.inner; }
         // Comparison with end iterator is always false
         friend bool operator!=(T *, const iterator&) { return false; }
         friend bool operator!=(const iterator&, T *) { return false; }
      private:
         node * inner = nullptr;
      };
   public:
      using iterator       = struct iterator;
      using const_iterator = const iterator *;
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

      T * end()       { return nullptr; }
      T * end() const { return nullptr; }

      allocator_type get_allocator() const { return allocator_type(); }

   private:
      node * head = nullptr;
   };
}

#endif
