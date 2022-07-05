#pragma once

#include "includes.hpp"
#include "manipulated_code.hpp"
#include "manipulated_values.hpp"


class mem_manip_lib
{
private:
        
    std::uint32_t base{ reinterpret_cast<std::uint32_t>(GetModuleHandleA(nullptr)) };
    const std::string_view dllname{ nullptr };

    void* mem_address{ nullptr };
    bool console_attached{ false };

    HMODULE mod_handle{ nullptr };
    DWORD old_prot{ 0 };

public:

    template<typename...var_arg>
    void dbg_log(std::string const& dbg_message, var_arg...fmt_args) const;

    template<typename... var_arg>
    void dbg_err(std::string const& dbg_error, var_arg...fmt_args) const;

    template<typename stl_container_t>
    stl_container_t mem_read_bytes() const;

    template<typename value_t>
    void mem_set_val(value_t val) const;

    template<typename ret_t>
    ret_t mem_read_val() const;

    template<std::uint32_t sz>
    manipulated_code mem_tramp_hook(const std::uint32_t func_addr);

    template<typename data_t, std::size_t sz>
    std::array<data_t, sizeof(data_t) * sz> mem_read_dyn();
    
    std::uint32_t get_va() const;
    std::uint32_t get_rva() const;

    std::uint8_t mem_read_byte(bool signednesss) const;
    std::string mem_read_string(const std::uint32_t string_sz) const;

    std::vector< std::uint8_t > mem_read_func_bytes() const;

    bool alloc_console();
    bool free_console();

    void reloc_rva(const std::uint32_t mem_address);
    void set_va(const std::uint32_t mem_address);


    void set_rwx(const std::size_t sz);
    void unset_rwx(const std::size_t sz);

    void mem_set_nop(const std::size_t sz);

    bool mem_set_bytes(const std::size_t szbyte, std::span<std::uint8_t> byte_arr);
    bool mem_set_bytes(const std::size_t szbyte, std::uint8_t byte);

    void unload();

    explicit mem_manip_lib(HMODULE hmod, const std::string_view dll_name, bool alloc_console_f = false) 
        : dllname(dll_name), mod_handle(hmod) 
    {
        if (alloc_console_f) 
        {   
            alloc_console();
            dbg_log( "\ninitialized %s\n", this->dllname );
        }
        else if (GetConsoleWindow()) 
        {
            dbg_log("\ninitialized %s\n", this->dllname);
        }
    }
};
    
template< typename stl_container_t >
stl_container_t mem_manip_lib::mem_read_bytes() const 
{
    auto bytes_read = stl_container_t();

    for ( auto k = 0u; k < bytes_read.size(); ++k )
    {
        bytes_read[k] = ( * ( reinterpret_cast< std::uint8_t* >( reinterpret_cast< std::uint32_t >( this->mem_address ) + k) ) );
    }

    return bytes_read;
}

template<typename... var_arg>
void mem_manip_lib::dbg_log(std::string const& dbg_message, var_arg... fmt_args) const 
{
    std::string dbg_err_prefix = "[+] ";
    std::printf((dbg_err_prefix + dbg_message).c_str() , fmt_args...);
}


template<typename... var_arg>
void mem_manip_lib::dbg_err(std::string const& dbg_message, var_arg... fmt_args) const 
{

    std::string dbg_err_prefix = "[!] ";
    std::printf((dbg_err_prefix + dbg_message).c_str() , fmt_args...);

}


template<typename value_t>
void mem_manip_lib::mem_set_val(value_t val) const
{

    *static_cast<value_t*>(this->mem_address) = val;

}

template<typename ret_t>
ret_t mem_manip_lib::mem_read_val() const {

    return *reinterpret_cast<ret_t*>(this->mem_address);

}

template<typename data_t, std::size_t sz>
std::array<data_t, sizeof(data_t) * sz> mem_manip_lib::mem_read_dyn() {

    std::array<data_t, sizeof(data_t) * sz> data;

    for (auto idx = 0u; idx < sizeof(data_t) * sz; idx += sizeof(data_t)) {

        data[(idx / sizeof(data_t))] = *reinterpret_cast<data_t*>(reinterpret_cast<std::uint32_t>(this->mem_address) + idx);

    }

    return data;
}


template<std::uint32_t sz>
manipulated_code mem_manip_lib::mem_tramp_hook(const std::uint32_t new_func)
{

    constexpr auto jmp_instr_size = 5;

    if (sz < jmp_instr_size)
    {
        return nullptr;
    }


    const auto old_bytes = mem_read_bytes<std::array<std::uint8_t, sz>>();
    auto new_bytes = std::to_array<std::uint8_t>({ 0xE9, 0x00, 0x00, 0x00, 0x00 });

    *reinterpret_cast<std::uint32_t*>(new_bytes[1]) = new_func;
    
    set_rwx(sz);
    std::memset(this->mem_address, 0x90, sz);

    const auto rel_addr = (new_func - reinterpret_cast<std::uint32_t>(this->mem_address)) - jmp_instr_size;


    *(static_cast<std::uint8_t*>(this->mem_address)) = 0xE9;
    *(reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint32_t>(this->mem_address) + 1)) = rel_addr;

    unset_rwx(sz);

    manipulated_code code_class(old_bytes, new_bytes, this->mem_address);
    return code_class;
}


