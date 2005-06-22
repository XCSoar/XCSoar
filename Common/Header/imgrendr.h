/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Tue Feb 10 13:46:55 1998
 */
/* Compiler settings for .\imgrendr.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __imgrendr_h__
#define __imgrendr_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IImageRender_FWD_DEFINED__
#define __IImageRender_FWD_DEFINED__
typedef interface IImageRender IImageRender;
#endif 	/* __IImageRender_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IImageRender_INTERFACE_DEFINED__
#define __IImageRender_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IImageRender
 * at Tue Feb 10 13:46:55 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IImageRender;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("59032090-154B-11d1-A9BF-006097DE299B")
    IImageRender : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Draw( 
            HDC hdc,
            RECT __RPC_FAR *lpRect) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetBitmap( 
            HBITMAP __RPC_FAR *phBitmap,
            BOOL fTake) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetOrigWidth( 
            int __RPC_FAR *piWidth) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetOrigHeight( 
            int __RPC_FAR *piHeight) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetBits( 
            unsigned char __RPC_FAR *__RPC_FAR *ppbBits) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ImageFail( 
            BOOL __RPC_FAR *pbFail) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IImageRenderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IImageRender __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IImageRender __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IImageRender __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Draw )( 
            IImageRender __RPC_FAR * This,
            HDC hdc,
            RECT __RPC_FAR *lpRect);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBitmap )( 
            IImageRender __RPC_FAR * This,
            HBITMAP __RPC_FAR *phBitmap,
            BOOL fTake);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOrigWidth )( 
            IImageRender __RPC_FAR * This,
            int __RPC_FAR *piWidth);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOrigHeight )( 
            IImageRender __RPC_FAR * This,
            int __RPC_FAR *piHeight);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBits )( 
            IImageRender __RPC_FAR * This,
            unsigned char __RPC_FAR *__RPC_FAR *ppbBits);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ImageFail )( 
            IImageRender __RPC_FAR * This,
            BOOL __RPC_FAR *pbFail);
        
        END_INTERFACE
    } IImageRenderVtbl;

    interface IImageRender
    {
        CONST_VTBL struct IImageRenderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IImageRender_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IImageRender_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IImageRender_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IImageRender_Draw(This,hdc,lpRect)	\
    (This)->lpVtbl -> Draw(This,hdc,lpRect)

#define IImageRender_GetBitmap(This,phBitmap,fTake)	\
    (This)->lpVtbl -> GetBitmap(This,phBitmap,fTake)

#define IImageRender_GetOrigWidth(This,piWidth)	\
    (This)->lpVtbl -> GetOrigWidth(This,piWidth)

#define IImageRender_GetOrigHeight(This,piHeight)	\
    (This)->lpVtbl -> GetOrigHeight(This,piHeight)

#define IImageRender_GetBits(This,ppbBits)	\
    (This)->lpVtbl -> GetBits(This,ppbBits)

#define IImageRender_ImageFail(This,pbFail)	\
    (This)->lpVtbl -> ImageFail(This,pbFail)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IImageRender_Draw_Proxy( 
    IImageRender __RPC_FAR * This,
    HDC hdc,
    RECT __RPC_FAR *lpRect);


void __RPC_STUB IImageRender_Draw_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IImageRender_GetBitmap_Proxy( 
    IImageRender __RPC_FAR * This,
    HBITMAP __RPC_FAR *phBitmap,
    BOOL fTake);


void __RPC_STUB IImageRender_GetBitmap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IImageRender_GetOrigWidth_Proxy( 
    IImageRender __RPC_FAR * This,
    int __RPC_FAR *piWidth);


void __RPC_STUB IImageRender_GetOrigWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IImageRender_GetOrigHeight_Proxy( 
    IImageRender __RPC_FAR * This,
    int __RPC_FAR *piHeight);


void __RPC_STUB IImageRender_GetOrigHeight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IImageRender_GetBits_Proxy( 
    IImageRender __RPC_FAR * This,
    unsigned char __RPC_FAR *__RPC_FAR *ppbBits);


void __RPC_STUB IImageRender_GetBits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IImageRender_ImageFail_Proxy( 
    IImageRender __RPC_FAR * This,
    BOOL __RPC_FAR *pbFail);


void __RPC_STUB IImageRender_ImageFail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IImageRender_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HBITMAP_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HBITMAP __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HBITMAP_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HBITMAP __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HBITMAP_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HBITMAP __RPC_FAR * ); 
void                      __RPC_USER  HBITMAP_UserFree(     unsigned long __RPC_FAR *, HBITMAP __RPC_FAR * ); 

unsigned long             __RPC_USER  HDC_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HDC __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HDC_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HDC __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HDC_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HDC __RPC_FAR * ); 
void                      __RPC_USER  HDC_UserFree(     unsigned long __RPC_FAR *, HDC __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
