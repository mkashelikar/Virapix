#ifndef _CElfReader_
#define _CElfReader_

#include "CException.h"

#include <string>
#include <memory>

class CElfReader;

class _IElfReader
{
    friend class CElfReader;

    private:
        virtual void _Reset() = 0;
        virtual bool _ReadHeaders(std::string& error) = 0;
        virtual unsigned long _GetMemOffset(const std::string& symbol, std::string& error) const = 0;
        virtual unsigned long _GetFileOffset(const std::string& symbol, std::string& error) const = 0;
};

template <typename ElfEHdr, typename ElfSHdrRecord, typename ElfSymTabRecord>
class _CElfReader : public _IElfReader
{
    friend class CElfReader;

    private:
        void* m_pMemoryMap;
        const ElfEHdr* m_pElfHdr;
        const ElfSHdrRecord* m_pSectionHdrTab;
        const ElfSymTabRecord* m_pSymTab;
        const char* m_pSectionStrTab;
        const char* m_pSymStrTab;
        unsigned int m_symTabEntries;

    private:
        _CElfReader(void* memMap);
        virtual ~_CElfReader();

        virtual void _Reset();
        virtual bool _ReadHeaders(std::string& error);
        virtual unsigned long _GetMemOffset(const std::string& symbol, std::string& error) const;
        virtual unsigned long _GetFileOffset(const std::string& symbol, std::string& error) const;

        bool _IsValidFormat() const;
        bool _SeekHdrsOffset(std::string& error);
        bool _SeekSymTab(std::string& error);
};

class CElfReader
{
    public:
        CElfReader();
        CElfReader(const std::string& path);
        ~CElfReader();

        void Open(const std::string& path);
        void Close();
        unsigned long GetMemOffset(const std::string& symbol) const;
        unsigned long GetFileOffset(const std::string& symbol) const;

        operator bool() const;
        bool operator !() const;

    private:
        unsigned long m_mapSize;
        void* m_pMemoryMap;
        std::auto_ptr<_IElfReader> m_pElfReader;

    private:
        int _GetBitness() const;
        void _Reset();
        bool _MemMap(const std::string& path);
        void _MemUnmap();
};

inline CElfReader::operator bool() const
{
    return !!m_pMemoryMap;
}

inline bool CElfReader::operator !() const
{
    return !m_pMemoryMap;
}

#endif
