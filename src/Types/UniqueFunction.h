#pragma once

#include <memory>

// Basically an std::function without CopyConstructible requirement. Similar to std::move_only_function
class UniqueFunction {
public :
    UniqueFunction() = default;
    
    template<typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, UniqueFunction>>>
    UniqueFunction(F&& f) : callable(std::make_unique<Model<std::decay_t<F>>>(std::forward<F>(f))) {}
    
    UniqueFunction(UniqueFunction&&) noexcept = default;
    UniqueFunction& operator=(UniqueFunction&&) noexcept = default;

    UniqueFunction(const UniqueFunction&) = delete;
    UniqueFunction& operator=(const UniqueFunction&) = delete;
    
    void operator()() {if (callable) callable->Invoke(); }
    explicit operator bool() const {return callable != nullptr; }

private:
        struct Concept {
            virtual ~Concept() = default;
            virtual void Invoke() = 0;
        };

        template<typename F>
        struct Model final : Concept {
            F func;
            Model(F&& f) : func(std::move(f)) {}
            void Invoke() override { func();}
        };

        std::unique_ptr<Concept> callable;
};

// A move-only, type-erased, no-argument callable. Substitute for std::move_only_function
// std::function requires its target to be CopyConstructible, even if the std::function
// itself is never copied - hard requirement. Which is Fundamentally incompatible with what every async job needs
// to do: move a WShell::Pidl (or a WShell::Directory, which owns one) into its capture
// list. Both are move-only by design. A lambda capturing either has its copy constructor implicitly deleted, 
// so it can never be wrapped in a std::function.