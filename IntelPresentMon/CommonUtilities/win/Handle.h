#pragma once

namespace pmon::util::win
{
    class Handle
    {
    public:
        // types
        // define handle alias compatible with winapi HANDLE to avoid unnecessary Window.h inclusion
        using HandleType = void*;
        // functions
        Handle();
        explicit Handle(HandleType handle);
        ~Handle();
        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;
        Handle(Handle&& other) noexcept;
        Handle& operator=(Handle&& other) noexcept;
        operator HandleType() const;
        HandleType Get() const;
        // empties this container (closes owned handle if any)
        void Clear();
        Handle Clone() const;
        // empties this resource, returning owned handle value without closing
        HandleType Release();
        HandleType* ClearAndGetAddressOf();
        operator bool() const noexcept;
        static Handle CreateCloned(HandleType handle);
    private:
        HandleType handle_ = nullptr;
    };
}
