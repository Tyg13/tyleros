#ifndef UTIL_LIST_H
#define UTIL_LIST_H

#include "allocator.h"

namespace kstd {
   template <typename T, typename Allocator = kstd::allocator<T>>
   class list {
      struct node {
         T    data;
         node * next = nullptr;
         node * operator++() const { return next; }
      };
   public:
      using iterator       =       node *;
      using const_iterator = const node *;
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
         const auto new_node = [&]() {
            auto new_node = node_allocator().allocate(sizeof(node));
            *new_node = node { .data = value, .next = nullptr };
            return new_node;
         };
         if (head == nullptr) {
            head = new_node();
         } else {
            while (head->next) {
               head = head->next;
            }
            head->next = new_node();
         }
      }

      iterator       begin()       { return head; }
      const_iterator begin() const { return head; }

      iterator       end()       { return nullptr; }
      const_iterator end() const { return nullptr; }

      allocator_type get_allocator() const { return allocator_type(); }

   private:
      node * head = nullptr;
   };
}

#endif
