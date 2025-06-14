#include <typeinfo>
#include <iostream>
namespace byBit
{
    class Any
    {
    private:
        class holder
        {
        public:
            virtual ~holder() {}
            virtual holder *clone() = 0;
            virtual const std::type_info &type() = 0;
        };

        template <class T>
        class placeholder : public holder
        {
        public:
            T _val;

        public:
            placeholder<T>(const T &val) : _val(val) {}
            virtual ~placeholder<T>() override {}
            virtual holder *clone() override { return new placeholder<T>(_val); }
            virtual const std::type_info &type() { return typeid(_val); }
        };

    private:
        holder *_dummy;

    public:
        Any() : _dummy(nullptr) {}
        template <class T>
        Any(const T &val) : _dummy(new placeholder<T>(val)) {}
        Any(const Any &other) : _dummy(other._dummy ? (other._dummy->clone()) : nullptr) {}
        ~Any() { delete _dummy; }
        void swap(Any &other)
        {
            std::swap(_dummy, other._dummy);
        }
        Any &operator=(const Any &other)
        {
            Any(other).swap(*this);
            return *this;
        }
        template <class T>
        Any &operator=(const T &val)
        {
            Any(val).swap(*this);
            return *this;
        }
        template <class T>
        T *get()
        {
            if (_dummy == nullptr || _dummy->type() != typeid(T))
            {
                return nullptr;
            }
            return &((placeholder<T> *)_dummy)->_val;
            // return &(((placeholder<T> *)_dummy)->_val);
            // return &((placeholder<T>*)_dummy)->_val;
        }
    };

}