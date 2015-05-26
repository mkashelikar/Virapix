#include "CElfReader.h"
#include "CException.h"
#include "ElfDefs.h"

#include <elf.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

////////////////////////////////////////////////// Inline definitions ////////////////////////////////////////////////////

inline int CElfReader::_GetBitness() const
{
    const unsigned char* elfID = (const unsigned char*) m_pMemoryMap;
    return (elfID[EI_CLASS] == ELFCLASS64) ? 64 : (elfID[EI_CLASS] == ELFCLASS32) ? 32 : -1;
}

////////////////////////////////////////////////// Public CElfReader definitions ////////////////////////////////////////////////////

CElfReader::CElfReader()
{
    _Reset();
}

CElfReader::CElfReader(const std::string& path)
{
    _Reset();
    Open(path);
}

CElfReader::~CElfReader()
{
    Close();
}

void CElfReader::Open(const std::string& path)
{
    if ( m_pMemoryMap )
    {
        return;
    }

    if ( _MemMap(path) )
    {
        int bitness = _GetBitness();

        if ( bitness == -1 )
        {
            throw CException(ERR_NOELF);
        }

        if ( bitness == 64 )
        {
            m_pElfReader.reset(new _CElfReader<Elf64_Ehdr, Elf64_Shdr, Elf64_Sym>(m_pMemoryMap));
        }
        else
        {
            m_pElfReader.reset(new _CElfReader<Elf32_Ehdr, Elf32_Shdr, Elf32_Sym>(m_pMemoryMap)); 
        }
                    
        std::string error;

        if ( !m_pElfReader->_ReadHeaders(error) )
        {
            throw CException(error);
        }
    }
    else
    {
        throw CException(ERR_NOINIT);
    }
}

void CElfReader::Close()
{
    _MemUnmap();
    _Reset();
}

// This function returns the symbol offset in the memory image of the binary. Note that Linux creates separate memory maps for mappable sections in
// the elf file. For correct results therefore, the caller must apply this offset to the memory address that corresponds to the start of the elf
// section that contains the symbol. Since it's all virtual memory, the offset calculated below is in no way related to the actual size of the
// binary. In fact it's not uncommon to see the offset far greater than the file size itself!
// TODO: Will this work for symbols in .data section? A quick look at the section headers reveals that st_value for a symbol table entry has 
// a different interpretation for symbols coming from .data and .text sections.
unsigned long CElfReader::GetMemOffset(const std::string& symbol) const
{
    std::string error;
    unsigned long offset = m_pElfReader->_GetMemOffset(symbol, error);

    if ( offset == (unsigned long) -1 )
    {
        CException ex(error);
        ex << symbol << ": " << errMsg;
        throw ex;
    }

    return offset;
}

// This function returns the symbol offset in an elf file on disk. The offset is an absolute value.
unsigned long CElfReader::GetFileOffset(const std::string& symbol) const
{
    std::string error;
    unsigned long offset = m_pElfReader->_GetFileOffset(symbol, error);

    if ( offset == (unsigned long) -1 )
    {
        CException ex(error);
        ex << symbol << ": " << errMsg;
        throw ex;
    }

    return offset;
}

/////////////////////////////////////////////// Private CElfReader definitions /////////////////////////////////////////////

void CElfReader::_Reset()
{
    m_mapSize = 0ul;
    m_pMemoryMap = 0;

    if ( m_pElfReader.get() )
    {
        m_pElfReader->_Reset();
    }
}

bool CElfReader::_MemMap(const std::string& path)
{
    int fd = open(path.c_str(), O_RDONLY);

    if ( fd != -1 )
    {
        m_mapSize = lseek(fd, 0, SEEK_END);
        m_pMemoryMap = mmap(0, m_mapSize, PROT_READ, MAP_PRIVATE, fd, 0);

        if ( m_pMemoryMap == MAP_FAILED )
        {
            m_pMemoryMap = 0;
            m_mapSize = 0ul;
        }

        close(fd);
    }

    return !!m_pMemoryMap;
}

void CElfReader::_MemUnmap()
{
    // No recording of error for munmap() because that would overwrite the error string stored by previously failed functions. 
    if ( m_pMemoryMap )
    {
        munmap(m_pMemoryMap, m_mapSize);
        m_pMemoryMap = 0;
        m_mapSize = 0ul;
    }
}

/////////////////////////////////////////////// Private _CElfReader definitions /////////////////////////////////////////////

template <typename ElfEHdr, typename ElfSHdrRecord, typename ElfSymTabRecord>
_CElfReader<ElfEHdr, ElfSHdrRecord, ElfSymTabRecord>::_CElfReader(void* memMap) : m_pMemoryMap(memMap)
{
    _Reset();
}

template <typename ElfEHdr, typename ElfSHdrRecord, typename ElfSymTabRecord>
_CElfReader<ElfEHdr, ElfSHdrRecord, ElfSymTabRecord>::~_CElfReader()
{
}

template <typename ElfEHdr, typename ElfSHdrRecord, typename ElfSymTabRecord>
void _CElfReader<ElfEHdr, ElfSHdrRecord, ElfSymTabRecord>::_Reset()
{
    m_pElfHdr = 0;
    m_pSectionHdrTab = 0;
    m_pSymTab = 0;
    m_pSectionStrTab = 0;
    m_pSymStrTab = 0;
    m_symTabEntries = 0;
}

template <typename ElfEHdr, typename ElfSHdrRecord, typename ElfSymTabRecord>
unsigned long _CElfReader<ElfEHdr, ElfSHdrRecord, ElfSymTabRecord>::_GetMemOffset(const std::string& symbol, std::string& error) const
{
    if ( !m_pSymTab || !m_pSymStrTab )
    {
        error = ERR_NOSYMTAB;
        return -1;
    }

    const ElfSymTabRecord* pTemp = m_pSymTab;
    unsigned long ret = (unsigned long) -1;

    for ( int count = 0; count < m_symTabEntries; ++count, ++pTemp )
    {
        const char* symName = m_pSymStrTab + pTemp->st_name;

        if ( symbol == symName )
        {
            ret = pTemp->st_value;
            break;
        }
    }

    if ( ret == -1 )
    {
        error = ERR_NOSYMBOL;
    }

    return ret;
}

template <typename ElfEHdr, typename ElfSHdrRecord, typename ElfSymTabRecord>
unsigned long _CElfReader<ElfEHdr, ElfSHdrRecord, ElfSymTabRecord>::_GetFileOffset(const std::string& symbol, std::string& error) const
{
    if ( !m_pSymTab || !m_pSymStrTab )
    {
        error = ERR_NOSYMTAB;
        return -1;
    }

    const ElfSymTabRecord* pTemp = m_pSymTab;
    unsigned long sectionOffset = (unsigned long) -1;
    unsigned long ret = (unsigned long) -1;

    for ( int count = 0; count < m_symTabEntries; ++count, ++pTemp )
    {
        const char* symName = m_pSymStrTab + pTemp->st_name;

        if ( symbol == symName )
        {
            const ElfSHdrRecord* pSectionHdrTab = m_pSectionHdrTab + pTemp->st_shndx;
            sectionOffset = pTemp->st_value - pSectionHdrTab->sh_addr;
            ret = pSectionHdrTab->sh_offset + sectionOffset;
            break;
        }
    }

    if ( ret == -1 )
    {
        error = ERR_NOSYMBOL;
    }

    return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Currently we support only executable and shared object files; no support for relocatable binaries yet.
template <typename ElfEHdr, typename ElfSHdrRecord, typename ElfSymTabRecord>
bool _CElfReader<ElfEHdr, ElfSHdrRecord, ElfSymTabRecord>::_IsValidFormat() const
{
    if ( m_pElfHdr )
    {
        const unsigned char* elfID = m_pElfHdr->e_ident;

        return (elfID[EI_MAG0] == ELFMAG0) && (elfID[EI_MAG1] == ELFMAG1) && (elfID[EI_MAG2] == ELFMAG2) 
            && (elfID[EI_MAG3] == ELFMAG3) && ( (elfID[EI_CLASS] == ELFCLASS32) || (elfID[EI_CLASS] == ELFCLASS64) ) 
            && ( (m_pElfHdr->e_type == ET_DYN) || (m_pElfHdr->e_type == ET_EXEC) );
    }

    return false;
}

template <typename ElfEHdr, typename ElfSHdrRecord, typename ElfSymTabRecord>
bool _CElfReader<ElfEHdr, ElfSHdrRecord, ElfSymTabRecord>::_ReadHeaders(std::string& error)
{
    bool status = false;

    error = "";
    m_pElfHdr = reinterpret_cast<const ElfEHdr*> (m_pMemoryMap);

    if ( _IsValidFormat() )
    {
        status = _SeekHdrsOffset(error);
    }
    else
    {
        m_pElfHdr = 0;
        error = ERR_NOELF;
    }

    return status;
}

template <typename ElfEHdr, typename ElfSHdrRecord, typename ElfSymTabRecord>
bool _CElfReader<ElfEHdr, ElfSHdrRecord, ElfSymTabRecord>::_SeekHdrsOffset(std::string& error)
{
    bool status = false;

    error = "";

    if ( !m_pElfHdr->e_shoff )
    {
        error = ERR_NOSHDRTAB;
    }
    else if ( m_pElfHdr->e_shstrndx == SHN_UNDEF )
    {
        error = ERR_NOSHDRSTRTAB;
    }
    else
    {
        const char* pElfStart = reinterpret_cast<const char*> (m_pMemoryMap);
        m_pSectionHdrTab = reinterpret_cast<const ElfSHdrRecord*>(pElfStart + m_pElfHdr->e_shoff);

        const ElfSHdrRecord* pTemp = m_pSectionHdrTab + m_pElfHdr->e_shstrndx;
        m_pSectionStrTab = pElfStart + pTemp->sh_offset;

        status = _SeekSymTab(error);
    }

    if ( error.size() )
    {
        m_pElfHdr = 0;
        m_pMemoryMap = 0;
    }

    return status;
}

template <typename ElfEHdr, typename ElfSHdrRecord, typename ElfSymTabRecord>
bool _CElfReader<ElfEHdr, ElfSHdrRecord, ElfSymTabRecord>::_SeekSymTab(std::string& error)
{
    error = "";

    const char* pElfStart = reinterpret_cast<const char*> (m_pMemoryMap);
    const ElfSHdrRecord* pTemp = m_pSectionHdrTab;
    const ElfSHdrRecord* pSymStrTabRecord = 0;

    for ( int count = 0; count < m_pElfHdr->e_shnum; ++count, ++pTemp )
    {
        if ( pTemp->sh_type == SHT_SYMTAB || pTemp->sh_type == SHT_DYNSYM )
        {
            m_pSymTab = reinterpret_cast<const ElfSymTabRecord*> (pElfStart + pTemp->sh_offset);
            m_symTabEntries = pTemp->sh_size / pTemp->sh_entsize;

            // The associated string table sections: .strtab for SHT_SYMTAB and .dynstr for SHT_DYNSYM
            pSymStrTabRecord = m_pSectionHdrTab + pTemp->sh_link;
            m_pSymStrTab = pElfStart + pSymStrTabRecord->sh_offset;

            // SHT_SYMTAB takes precedence since it's a superset of SHT_DYNSYM. So if we encounter SHT_DYNSYM first, we continue until either we
            // hit SHT_SYMTAB (overwriting values we gathered for SHT_DYNSYM earlier) or run out of section header entries in which case we
            // consider SHT_DYNSYM instead.
            if ( pTemp->sh_type == SHT_SYMTAB )
            {
                break;
            }
        }
    }

    if ( !m_pSymTab )
    {
        error = ERR_NOSYMTAB;
        _Reset();
        return false;
    }

    return true;
}
