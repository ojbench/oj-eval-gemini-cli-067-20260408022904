#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

class RefCellError : public std::runtime_error {
public:
    explicit RefCellError(const std::string& message) : std::runtime_error(message) {}
    virtual ~RefCellError() = default;
};

class BorrowError : public RefCellError {
public:
    explicit BorrowError(const std::string& message) : RefCellError(message) {}
};

class BorrowMutError : public RefCellError {
public:
    explicit BorrowMutError(const std::string& message) : RefCellError(message) {}
};

class DestructionError : public RefCellError {
public:
    explicit DestructionError(const std::string& message) : RefCellError(message) {}
};

template <typename T>
class RefCell {
private:
    T value;
    int borrow_count; // 0: no borrow, >0: immutable borrows, -1: mutable borrow

public:
    class Ref;
    class RefMut;

    explicit RefCell(const T& initial_value) : value(initial_value), borrow_count(0) {}
    explicit RefCell(T&& initial_value) : value(std::move(initial_value)), borrow_count(0) {}
    
    RefCell(const RefCell&) = delete;
    RefCell& operator=(const RefCell&) = delete;
    RefCell(RefCell&&) = delete;
    RefCell& operator=(RefCell&&) = delete;

    Ref borrow() const {
        if (borrow_count < 0) {
            throw BorrowError("Already mutably borrowed");
        }
        return Ref(&value, &const_cast<RefCell*>(this)->borrow_count);
    }

    std::optional<Ref> try_borrow() const {
        if (borrow_count < 0) {
            return std::nullopt;
        }
        return Ref(&value, &const_cast<RefCell*>(this)->borrow_count);
    }

    RefMut borrow_mut() {
        if (borrow_count != 0) {
            throw BorrowMutError("Already borrowed");
        }
        return RefMut(&value, &borrow_count);
    }

    std::optional<RefMut> try_borrow_mut() {
        if (borrow_count != 0) {
            return std::nullopt;
        }
        return RefMut(&value, &borrow_count);
    }

    class Ref {
    private:
        const T* ptr;
        int* count_ptr;

    public:
        Ref() : ptr(nullptr), count_ptr(nullptr) {}

        Ref(const T* p, int* c) : ptr(p), count_ptr(c) {
            if (count_ptr) {
                (*count_ptr)++;
            }
        }

        ~Ref() {
            if (count_ptr) {
                (*count_ptr)--;
            }
        }

        const T& operator*() const {
            return *ptr;
        }

        const T* operator->() const {
            return ptr;
        }

        Ref(const Ref& other) : ptr(other.ptr), count_ptr(other.count_ptr) {
            if (count_ptr) {
                (*count_ptr)++;
            }
        }

        Ref& operator=(const Ref& other) {
            if (this != &other) {
                if (count_ptr) {
                    (*count_ptr)--;
                }
                ptr = other.ptr;
                count_ptr = other.count_ptr;
                if (count_ptr) {
                    (*count_ptr)++;
                }
            }
            return *this;
        }

        Ref(Ref&& other) noexcept : ptr(other.ptr), count_ptr(other.count_ptr) {
            other.ptr = nullptr;
            other.count_ptr = nullptr;
        }

        Ref& operator=(Ref&& other) noexcept {
            if (this != &other) {
                if (count_ptr) {
                    (*count_ptr)--;
                }
                ptr = other.ptr;
                count_ptr = other.count_ptr;
                other.ptr = nullptr;
                other.count_ptr = nullptr;
            }
            return *this;
        }
    };

    class RefMut {
    private:
        T* ptr;
        int* count_ptr;

    public:
        RefMut() : ptr(nullptr), count_ptr(nullptr) {}

        RefMut(T* p, int* c) : ptr(p), count_ptr(c) {
            if (count_ptr) {
                *count_ptr = -1;
            }
        }

        ~RefMut() {
            if (count_ptr) {
                *count_ptr = 0;
            }
        }

        T& operator*() {
            return *ptr;
        }

        T* operator->() {
            return ptr;
        }

        RefMut(const RefMut&) = delete;
        RefMut& operator=(const RefMut&) = delete;

        RefMut(RefMut&& other) noexcept : ptr(other.ptr), count_ptr(other.count_ptr) {
            other.ptr = nullptr;
            other.count_ptr = nullptr;
        }

        RefMut& operator=(RefMut&& other) noexcept {
            if (this != &other) {
                if (count_ptr) {
                    *count_ptr = 0;
                }
                ptr = other.ptr;
                count_ptr = other.count_ptr;
                other.ptr = nullptr;
                other.count_ptr = nullptr;
            }
            return *this;
        }
    };

    ~RefCell() {
        if (borrow_count != 0) {
            throw DestructionError("Still borrowed when destructed");
        }
    }
};
