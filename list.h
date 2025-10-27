#pragma once
#include <iostream>

template<typename T>
class List {
private:
    // Node structure representing a single element in the list
    struct Node {
        T data;
        Node* next;
        Node(const T& value) : data(value), next(nullptr) {}
    };
    
    Node* head;
    int size;

public:
    // Iterator class for traversing the list
    class Iterator {
    private:
        Node* current;
    public:
        Iterator(Node* node) : current(node) {}
        
        T& operator*() { return current->data; }
        T* operator->() { return &(current->data); }
        
        Iterator& operator++() {
            if (current) current = current->next;
            return *this;
        }
        
        bool operator!=(const Iterator& other) const {
            return current != other.current;
        }
        
        bool operator==(const Iterator& other) const {
            return current == other.current;
        }
    };
    
    // Constructor — initializes an empty list
    List() : head(nullptr), size(0) {}
    
    // Destructor — clears all elements
    ~List() {
        clear();
    }
    
    // Adds a new element to the end of the list
    void push_back(const T& value) {
        Node* newNode = new Node(value);
        if (!head) {
            head = newNode;
        } else {
            Node* temp = head;
            while (temp->next) {
                temp = temp->next;
            }
            temp->next = newNode;
        }
        size++;
    }
    
    // Removes all elements from the list
    void clear() {
        while (head) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
        size = 0;
    }
    
    // Returns the number of elements
    int getSize() const { return size; }
    
    bool isEmpty() const { return head == nullptr; }
    
    Iterator begin() { return Iterator(head); }

    Iterator end() { return Iterator(nullptr); }
    
    // Accesses the element by index
    T& operator[](int index) {
        Node* temp = head;
        for (int i = 0; i < index && temp; i++) {
            temp = temp->next;
        }
        return temp->data;
    }
};
