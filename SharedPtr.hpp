#ifndef SHAREDPTR_HPP
#define SHAREDPTR_HPP

#include <iostream>
#include <mutex>
#include <atomic>

namespace cs540
{
    std::mutex mutexLock;

    template <typename T>
    class SharedPtr;

    template <class T>
    void valuePtrDestructor(const void *ptr)
    {
        delete static_cast<const T *>(ptr);
        ptr = nullptr;
    }
    //  Control Block
    class ControlBlock
    {
    public:
        void (*destructor)(const void *);
        ControlBlock() : valuePtr(nullptr)
        {
            refCount.store(0);
        }
        ControlBlock(void *ptr) : valuePtr(ptr)
        {
            refCount.store(0);
        }
        ~ControlBlock()
        {
            if (valuePtr != nullptr && destructor)
            {
                (*destructor)(valuePtr);
            }
        }
        void increment()
        {
            refCount.fetch_add(1);
        }
        void decrement()
        {
            refCount.fetch_sub(1);
        }
        int getRefCount()
        {
            return refCount.load();
        }

    private:
        std::atomic_int refCount;
        const void *valuePtr;
    };

    template <typename T>
    class SharedPtr
    {
    public:
        T *valuePtr;
        ControlBlock *refCounter;
        //      Constructors        //

        //  Constructs a smart pointer that points to null.
        SharedPtr() : valuePtr(nullptr), refCounter(new ControlBlock()) {}

        //  Constructs a smart pointer that points to the given object. The reference count is initialized to one.
        template <typename U>
        explicit SharedPtr(U *value) : valuePtr(value), refCounter(new ControlBlock(value))
        {

            if (refCounter)
            {
                refCounter->increment();
                refCounter->destructor = valuePtrDestructor<U>;
            }
        }

        //  If p is not null, then reference count of the managed object is incremented.

        //  If U * is not implicitly convertible to T *, use of the second constructor will
        //  result in a compile-time error when the compiler attempts to instantiate the member template.

        SharedPtr(const SharedPtr &p) : valuePtr(p.valuePtr), refCounter(p.refCounter)
        {

            if (refCounter)
            {
                refCounter->increment();
            }
        }

        template <typename U>
        SharedPtr(const SharedPtr<U> &p) : valuePtr(p.valuePtr), refCounter(p.refCounter)
        {

            if (refCounter)
            {
                refCounter->destructor = valuePtrDestructor<U>;
                refCounter->increment();
            }
        }

        //  Move the managed object from the given smart pointer. The reference count must remain unchanged.
        //  After this function, p must be null. This must work if U * is implicitly convertible to T *

        SharedPtr(SharedPtr &&p) : valuePtr(p.valuePtr), refCounter(p.refCounter)
        {

            p.valuePtr = nullptr;
            p.refCounter = nullptr;
        }

        template <typename U>
        SharedPtr(SharedPtr<U> &&p) : valuePtr(p.valuePtr), refCounter(p.refCounter)
        {

            if (refCounter)
            {
                refCounter->destructor = valuePtrDestructor<U>;
            }
            p.valuePtr = nullptr;
            p.refCounter = nullptr;
        }

        //      Assignment Operators        //

        //  Copy assignment. Must handle self assignment. Decrement reference count of current object, if any, and
        //  increment reference count of the given object. If U * is not implicitly convertible to T *, this will
        //  result in a syntax error. Note that both the normal assignment operator and a member template assignment
        //  operator must be provided for proper operation.

        SharedPtr &operator=(const SharedPtr &p)
        {

            if (this != &p)
            {
                refCounter->decrement();
                if (refCounter->getRefCount() <= 0)
                {
                    //  delete refcount
                    controlBlockdestructor();
                }

                valuePtr = p.valuePtr;
                refCounter = p.refCounter;

                if (refCounter != nullptr)
                {
                    refCounter->increment();
                }
            }

            return *this;
        }

        template <typename U>
        SharedPtr<T> &operator=(const SharedPtr<U> &p)
        {

            refCounter->decrement();
            if (refCounter->getRefCount() <= 0)
            {
                //  delete refcount
                controlBlockdestructor();
            }

            valuePtr = p.valuePtr;
            refCounter = p.refCounter;

            if (refCounter != nullptr)
            {
                refCounter->increment();
                refCounter->destructor = valuePtrDestructor<U>;
            }

            return *this;
        }

        //  Move assignment. After this operation, p must be null. The reference count associated with the object
        //  (if p is not null before the operation) will remain the same after this operation. This must compile
        //  and run correctly if U * is implicitly convertible to T *, otherwise, it must be a syntax error.
        SharedPtr &operator=(SharedPtr &&p)
        {

            refCounter->decrement();
            if (refCounter->getRefCount() <= 0)
            {
                //  delete refcount
                controlBlockdestructor();
            }

            valuePtr = p.valuePtr;
            refCounter = p.refCounter;

            p.valuePtr = nullptr;
            p.refCounter = nullptr;

            return *this;
        }
        template <typename U>
        SharedPtr &operator=(SharedPtr<U> &&p)
        {

            refCounter->decrement();
            if (refCounter->getRefCount() <= 0)
            {
                //  delete refcount
                controlBlockdestructor();
            }

            valuePtr = p.valuePtr;
            refCounter = p.refCounter;

            if (refCounter != nullptr)
            {
                refCounter->destructor = valuePtrDestructor<U>;
            }

            p.valuePtr = nullptr;
            p.refCounter = nullptr;

            return *this;
        }

        //      Destructor        //

        //  Decrement reference count of managed object. If the reference count is zero, delete the object.
        ~SharedPtr()
        {
            reset();
        }

        //      Modifiers        //

        //  The smart pointer is set to point to the null pointer. The reference count for
        //  the currently pointed to object, if any, is decremented.
        void reset()
        {

            if (refCounter)
            {
                refCounter->decrement();
                if (refCounter->getRefCount() <= 0)
                {
                    controlBlockdestructor();
                }
                valuePtr = nullptr;
                refCounter = nullptr;
            }
            else
            {
                valuePtr = nullptr;
                refCounter = nullptr;
            }
        }

        //  Replace owned resource with another pointer. If the owned resource has no other references,
        //  it is deleted. If p has been associated with some other smart pointer, the behavior is undefined.
        template <typename U>
        void reset(U *p)
        {

            if (refCounter)
            {
                refCounter->decrement();
                if (refCounter->getRefCount() <= 0)
                {
                    controlBlockdestructor();
                }
            }

            valuePtr = p;
            refCounter = new ControlBlock(p);

            if (p)
            {
                refCounter->increment();
                refCounter->destructor = valuePtrDestructor<U>;
            }
        }

        //      Observers        //

        //  Returns a pointer to the owned object. Note that this will be a pointer-to-const if T is a
        // const-qualified type.
        T *get() const
        {
            return valuePtr;
        }

        //  A reference to the pointed-to object is returned. Note that this will be a
        //  const-reference if T is a const-qualified type.
        T &operator*() const
        {
            return *valuePtr;
        }

        //  The pointer is returned. Note that this will be a pointer-to-const if T is a const-qualified type.
        T *operator->() const
        {
            return valuePtr;
        }

        //  Returns true if the SharedPtr is not null.
        explicit operator bool() const
        {
            return valuePtr != nullptr;
        }

    protected:
        void controlBlockdestructor()
        {
            if (refCounter != nullptr)
            {
                delete refCounter;
                refCounter = nullptr;
            }
        }
    };

    //      Non-Member Functions        //

    //  Returns true if the two smart pointers point to the same object or
    //  if they are both null. Note that implicit conversions may be applied.
    template <typename T1, typename T2>
    bool operator==(const SharedPtr<T1> &p1, const SharedPtr<T2> &p2)
    {
        return p1.valuePtr == p2.valuePtr;
    }

    //  Compare the SharedPtr against nullptr.
    template <typename T>
    bool operator==(const SharedPtr<T> &p1, std::nullptr_t p2)
    {
        return p1.valuePtr == nullptr;
    }
    template <typename T>
    bool operator==(std::nullptr_t p1, const SharedPtr<T> &p2)
    {
        return nullptr == p2.valuePtr;
    }

    //  True if the SharedPtrs point to different objects, or one points to null while the other does not.
    template <typename T1, typename T2>
    bool operator!=(const SharedPtr<T1> &p1, const SharedPtr<T2> &p2)
    {
        return p1.valuePtr == p2.valuePtr;
    }

    //  Compare the SharedPtr against nullptr.
    template <typename T>
    bool operator!=(const SharedPtr<T> &p1, std::nullptr_t p2)
    {
        return p1.valuePtr == nullptr;
    }
    template <typename T>
    bool operator!=(std::nullptr_t p1, const SharedPtr<T> &p2)
    {
        return nullptr == p2.valuePtr;
    }

    //  Convert sp by using static_cast to cast the contained pointer. It will result
    //  in a syntax error if static_cast cannot be applied to the relevant types.
    template <typename T, typename U>
    SharedPtr<T> static_pointer_cast(const SharedPtr<U> &sp)
    {
        SharedPtr<T> newSp = SharedPtr<T>();
        newSp.valuePtr = static_cast<T *>(sp.get());
        newSp.refCounter = sp.refCounter;

        newSp.refCounter->increment();

        return newSp;
    }

    //  Convert sp by using dynamic_cast to cast the contained pointer. It will result in a
    //  syntax error if dynamic_cast cannot be applied to the relevant types, and will result
    //  in a smart pointer to null if the dynamic type of the pointer in sp is not T *.
    template <typename T, typename U>
    SharedPtr<T> dynamic_pointer_cast(const SharedPtr<U> &sp)
    {
        SharedPtr<T> newSp = SharedPtr<T>();
        newSp.valuePtr = dynamic_cast<T *>(sp.get());
        newSp.refCounter = sp.refCounter;

        newSp.refCounter->increment();

        return newSp;
    }
}

#endif