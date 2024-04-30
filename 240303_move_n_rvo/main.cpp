#include <iostream>
#include <type_traits>
#include <utility>

using std::cerr, std::endl;

class TalkingClass {
public:
    TalkingClass() : id_(counter_++)
    {
        cerr << "TalkingClass() : id_ = " << id_ << endl;
    }

    TalkingClass(const TalkingClass& other) : id_(counter_++)
    {
        cerr << "TalkingClass(" << other.id_ << ") : id_ = " << id_ << endl;
    }

    TalkingClass(TalkingClass&& other) : id_(counter_++)
    {
        cerr << "TalkingClass(" << other.id_ << "&&) : id_ = " << id_ << endl;
        other.id_ *= -1;
    }

    ~TalkingClass() {
        cerr << "~TalkingClass() : id_ = " << id_ << endl;
    }

    TalkingClass& operator=(const TalkingClass&) = delete;
    TalkingClass& operator=(TalkingClass&&) = delete;

    int GetId() const noexcept
    {
        return id_;
    }

    static void Reset() {
        counter_ = 1;
    }

private:
    static inline int counter_ = 1;
    int id_;
};

const TalkingClass& SafeReturn(const TalkingClass& parameter) {
    return parameter;
}

TalkingClass SafeReturn(TalkingClass&& parameter) {
    return std::move(parameter);
}

template<typename T>
concept rvalue = std::is_rvalue_reference_v<T&&> && !std::is_const_v<std::remove_reference_t<T>>;
void foo([[maybe_unused]] rvalue auto&& t) {}

int main() {
    int x = 10;
    foo(std::move(x));

    {
        TalkingClass::Reset();
        cerr << R"(
        auto ReturnObj = []() -> TalkingClass {
            return TalkingClass();  // RVO
        };
        TalkingClass rvo = ReturnObj();)" << endl;
        auto ReturnObj = []() -> TalkingClass {
            return TalkingClass();  // RVO
        };
        TalkingClass rvo = ReturnObj();
        // TalkingClass() : id_ = 1
        // ~TalkingClass()
    }

    {
        TalkingClass::Reset();
        cerr << R"(
        auto ReturnMove = []() -> TalkingClass {
            return std::move(TalkingClass());  // move prevents RVO; move constructor
        };
        TalkingClass move = ReturnMove();)" << endl;
        auto ReturnMove = []() -> TalkingClass {
            return std::move(TalkingClass());  // move prevents RVO; move constructor
        };
        TalkingClass move = ReturnMove();
        // TalkingClass() : id_ = 1
        // TalkingClass(1&&) : id_ = 2
        // ~TalkingClass() : id_ = -1
        // ~TalkingClass() : id_ = 2
    }

    {
        TalkingClass::Reset();
        cerr << R"(
        [[maybe_unused]] auto ReturnRvalueReference = []() -> TalkingClass&& {
            return TalkingClass();  // warning: returning reference to temporary
        };
        TalkingClass move_reference = ReturnRvalueReference();)" << endl;
        [[maybe_unused]] auto ReturnRvalueReference = []() -> TalkingClass&& {
            return TalkingClass();  // warning: returning reference to temporary
        };
        // TalkingClass move_reference = ReturnRvalueReference();
        cerr << "main.cpp:96:33: runtime error: reference binding to null pointer of type 'struct TalkingClass'" << endl;
        cerr << "main.cpp:20:49: runtime error: member access within null pointer of type 'struct TalkingClass'" << endl;
    }

    {
        TalkingClass::Reset();
        cerr << R"(
        TalkingClass move_to_type = std::move(TalkingClass());
        cerr << "move_to_type.GetId() = " << move_to_type.GetId() << endl;)" << endl;
        TalkingClass move_to_type = std::move(TalkingClass());
        cerr << "move_to_type.GetId() = " << move_to_type.GetId() << endl;
        // TalkingClass() : id_ = 1
        // TalkingClass(1&&) : id_ = 2
        // ~TalkingClass() : id_ = -1
        // move_to_type.GetId() = 2
        // ~TalkingClass() : id_ = 2
    }

    {
        TalkingClass::Reset();
        cerr << R"(
        [[maybe_unused]] TalkingClass&& move_to_ref = std::move(TalkingClass());
        cerr << move_to_ref.GetId() << endl;)" << endl;
        [[maybe_unused]] TalkingClass&& move_to_ref = std::move(TalkingClass());
        // cerr << move_to_ref.GetId() << endl;
        cerr << "ERROR: AddressSanitizer: stack-use-after-scope" << endl;
        // ERROR: AddressSanitizer: stack-use-after-scope
    }

    {
        TalkingClass::Reset();
        cerr << R"(
        TalkingClass value;
        TalkingClass&& reference = std::move(value);
        cerr << "value.GetId() = " << value.GetId() << endl;
        cerr << "reference.GetId() = " << reference.GetId() << endl;)" << endl;
        TalkingClass value;
        TalkingClass&& reference = std::move(value);
        cerr << "value.GetId() = " << value.GetId() << endl;
        cerr << "reference.GetId() = " << reference.GetId() << endl;
        // TalkingClass() : id_ = 1
        // value.GetId() = 1
        // reference.GetId() = 1
        // ~TalkingClass() : id_ = 1
    }

    {
        TalkingClass::Reset();
        cerr << R"(
        TalkingClass&& accept_by_ref = static_cast<TalkingClass&&>(TalkingClass());
        cerr << "accept_by_ref.GetId() = " << accept_by_ref.GetId() << endl;)" << endl;
        TalkingClass&& accept_by_ref = static_cast<TalkingClass&&>(TalkingClass());
        cerr << "accept_by_ref.GetId() = " << accept_by_ref.GetId() << endl;
        // TalkingClass() : id_ = 1
        // accept_by_ref.GetId() = 1
        // ~TalkingClass() : id_ = 1
    }

    {
        TalkingClass::Reset();
        cerr << R"(
        auto ReturnParameterCopy = [](TalkingClass parameter) -> TalkingClass {
            return parameter;  // implicit move
        };
        TalkingClass x{};
        TalkingClass parameter_copy = ReturnParameterCopy(x);)" << endl;
        auto ReturnParameterCopy = [](TalkingClass parameter) -> TalkingClass {
            return parameter;  // implicit move
        };
        TalkingClass x{};
        TalkingClass parameter_copy = ReturnParameterCopy(x);
        // TalkingClass() : id_ = 1
        // TalkingClass(1) : id_ = 2
        // TalkingClass(2&&) : id_ = 3
        // ~TalkingClass() : id_ = -2
        // ~TalkingClass() : id_ = 3
        // ~TalkingClass() : id_ = 1
    }

    {
        TalkingClass::Reset();
        cerr << R"(
        auto ReturnParameterMove = [](TalkingClass& parameter) -> TalkingClass {
            return std::move(parameter)
        };
        TalkingClass x;
        TalkingClass parameter_move = ReturnParameterMove(x);)" << endl;
        auto ReturnParameterMove = [](TalkingClass& parameter) -> TalkingClass {
            return std::move(parameter);
        };
        TalkingClass x;
        TalkingClass parameter_move = ReturnParameterMove(x);
        // TalkingClass() : id_ = 1
        // TalkingClass(1&&) : id_ = 2
        // ~TalkingClass() : id_ = 2
        // ~TalkingClass() : id_ = -1
    }

    {
        TalkingClass::Reset();
        cerr << R"(
        auto ReturnParameterReference = [](TalkingClass& parameter) -> TalkingClass& {
            return parameter;
        };
        TalkingClass x;
        TalkingClass& parameter_reference = ReturnParameterReference(x);
        TalkingClass parameter_copy = ReturnParameterReference(x);
        cerr << "x.GetId() = " << x.GetId() << endl;;
        cerr << "parameter_reference.GetId() = " << parameter_reference.GetId() << endl;
        cerr << "parameter_copy.GetId() = " << parameter_copy.GetId() << endl;)" << endl;
        auto ReturnParameterReference = [](TalkingClass& parameter) -> TalkingClass& {
            return parameter;
        };
        TalkingClass x;
        TalkingClass& parameter_reference = ReturnParameterReference(x);
        TalkingClass parameter_copy = ReturnParameterReference(x);
        cerr << "x.GetId() = " << x.GetId() << endl;;
        cerr << "parameter_reference.GetId() = " << parameter_reference.GetId() << endl;
        cerr << "parameter_copy.GetId() = " << parameter_copy.GetId() << endl;
        // TalkingClass() : id_ = 1
        // TalkingClass(1) : id_ = 2
        // x.GetId() = 1
        // parameter_reference.GetId() = 1
        // parameter_copy.GetId() = 2
        // ~TalkingClass() : id_ = 2
        // ~TalkingClass() : id_ = 1
    }

    {
        TalkingClass::Reset();
        cerr << R"(
        auto UnsafeReturnRef = [](const TalkingClass& parameter) -> const TalkingClass& {
            return parameter;
        };
        [[maybe_unused]] const TalkingClass& ref = UnsafeReturnRef(TalkingClass());
        cerr << ref.GetId() << endl;)";
        auto UnsafeReturnRef = [](const TalkingClass& parameter) -> const TalkingClass& {
            return parameter;
        };
        [[maybe_unused]] const TalkingClass& ref = UnsafeReturnRef(TalkingClass());
        // cerr << ref.GetId() << endl;
        cerr << "AddressSanitizer: stack-use-after-scope" << endl;
    }

    {
        TalkingClass::Reset();
        cerr << R"(
        const TalkingClass& ref = SafeReturn(TalkingClass());
        cerr << "ref.GetId() = " << ref.GetId() << endl;)" << endl;;
        const TalkingClass& ref = SafeReturn(TalkingClass());
        cerr << "ref.GetId() = " << ref.GetId() << endl;
        // TalkingClass() : id_ = 1
        // TalkingClass(1&&) : id_ = 2
        // ~TalkingClass() : id_ = -1
        // ref.GetId() = 2
        // ~TalkingClass() : id_ = 2
    }

    {
        TalkingClass::Reset();
        cerr << R"(
        struct Wrap {
            TalkingClass contents;
            TalkingClass Get() {
                return std::move(contents);
            }
        };
        const TalkingClass& from_obj = Wrap().Get();
        cerr << "from_obj.GetId() = " << from_obj.GetId() << endl;)" << endl;
        struct Wrap {
            TalkingClass contents;
            TalkingClass Get() {
                return std::move(contents);
            }
        };
        const TalkingClass& from_obj = Wrap().Get();
        cerr << "from_obj.GetId() = " << from_obj.GetId() << endl;
        // TalkingClass() : id_ = 1
        // TalkingClass(1&&) : id_ = 2
        // ~TalkingClass() : id_ = -1
        // from_obj.GetId() = 2
        // ~TalkingClass() : id_ = 2
    }
}
