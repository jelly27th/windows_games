/* a ultra minimal working COM example */
/* Note: not fully COM compliant */

#include <stdio.h>
#include <malloc.h>
#include <iostream>

/*
    note: you must include this header it contains important constants
    you must use in COM programs.
*/
#include <objbase.h>

/* these were all generated with GUIDGEN.EXE */
// {B9B8ACE1-CE14-11d0-AE58-444553540000}
const IID IID_IX = 
{ 0xb9b8ace1, 0xce14, 0x11d0, { 0xae, 0x58, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };

// {B9B8ACE2-CE14-11d0-AE58-444553540000}
const IID IID_IY = 
{ 0xb9b8ace2, 0xce14, 0x11d0, { 0xae, 0x58, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };

// {B9B8ACE3-CE14-11d0-AE58-444553540000}
const IID IID_IZ = 
{ 0xb9b8ace3, 0xce14, 0x11d0, { 0xae, 0x58, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };

/* IUnknown interface */

// define the IX interface
interface IX : IUnknown
{
    virtual void __stdcall fx(void) = 0;
};

// define the IY interface
interface IY : IUnknown
{
    virtual void __stdcall fy(void) = 0;
};

/* classes and components */

// define the COM object class
class CCOM_OBJECT : public IX, public IY
{
    public:
        CCOM_OBJECT() : ref_count(0) { }
        ~CCOM_OBJECT() { }

    private:
       virtual HRESULT __stdcall QueryInterface(const IID& iid, void** iface);
       virtual ULONG __stdcall AddRef(void);
       virtual ULONG __stdcall Release(void);

       virtual void __stdcall fx(void) {
           std::cout << "Function fx has been called." << std::endl;
       }
       virtual void __stdcall fy(void) {
           std::cout << "Function fy has been called." << std::endl;
       }

       int ref_count;
};

/* class methods*/

/*
    this function basically casts the this pointer or th Iunknown
    pointer into the interface requested, notice the comparison with
    the GUIDs generated and defined in the begining of the program
*/
HRESULT __stdcall CCOM_OBJECT::QueryInterface(const IID& iid, void** iface)
{
    // requesting the IUnknown base interface
    if (iid == IID_IUnknown) {
        std::cout << "Requesting IUnknown interface." << std::endl;
        *iface = (IX*)(this);
    } 

    // maybe IX?
    if (iid == IID_IX) {
        std::cout << "Requesting IX interface." << std::endl;
        *iface = (IX*)(this);
    } 
    // maybe IY?
    else if (iid == IID_IY) {
        std::cout << "Requesting IY interface." << std::endl;
        *iface = (IY*)(this);
    }
    // can't fine it 
    else {
        std::cout << "Requesting unknown interface." << std::endl;
        *iface = NULL;
        return E_NOINTERFACE;
    }

    // if everything went well cast pointer to IUnknown and call addref()
    ((IUnknown*)(*iface))->AddRef();
    return S_OK;
}

/* increments reference count */
ULONG __stdcall CCOM_OBJECT::AddRef(void)
{
    std::cout << "Adding a reference." << std::endl;
    return ++ref_count;
}

/* decrements reference count */
ULONG __stdcall CCOM_OBJECT::Release(void)
{
    std::cout << "Deleting a reference." << std::endl;
    if (--ref_count == 0) {
        delete this;
        return 0;
    }
    else {
        return ref_count;
    }
}

/* 
    this is a very basic implementation of CoCreateInstance() 
    it creates an instance of the COM object, in this case 
    I decided to start with a pointer to IX -- IY would have
    done just as well
*/
IUnknown* CoCreateInstance(void)
{
    IUnknown* comm_obj = (IX*) new CCOM_OBJECT();
    std::cout << "Creating Comm object." << std::endl;
    
    // update reference
    comm_obj->AddRef();

    return comm_obj;
}

int main(void)
{
    // create the main COM object
    IUnknown* punknown = CoCreateInstance();

    // create two NULL pointers to the IX and IY interfaces
    IX* pix = NULL;
    IY* piy = NULL;

    // from the original COM object query for the IX interface
    punknown->QueryInterface(IID_IX, (void**)&pix);

    // try some of the methods of IX
    pix->fx();

    // release the interface
    pix->Release();

    // now query for the IY interface
    punknown->QueryInterface(IID_IY, (void**)&piy);
    piy->fy();
    piy->Release();

    // release the COM object itself
    punknown->Release();

    return 0;
}