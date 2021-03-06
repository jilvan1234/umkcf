/*
 * Process Hacker -
 *   internal object manager
 *
 * Copyright (C) 2009 wj32
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PH_REF_H
#define _PH_REF_H

#ifdef __cplusplus
extern "C" {
#endif

// Configuration

#define PHOBJ_SMALL_OBJECT_SIZE 48
#define PHOBJ_SMALL_OBJECT_COUNT 512

//#define PHOBJ_STRICT_CHECKS
#define PHOBJ_ALLOCATE_NEVER_NULL

/* Object flags */
#define PHOBJ_RAISE_ON_FAIL 0x00000001
#define PHOBJ_VALID_FLAGS 0x00000001

/* Object type flags */
#define PHOBJTYPE_USE_FREE_LIST 0x00000001
#define PHOBJTYPE_VALID_FLAGS 0x00000001

/* Object type callbacks */

/**
 * The delete procedure for an object type, called when
 * an object of the type is being freed.
 *
 * \param Object A pointer to the object being freed.
 * \param Flags Reserved.
 */
typedef VOID (NTAPI *PPH_TYPE_DELETE_PROCEDURE)(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

struct _PH_OBJECT_TYPE;
typedef struct _PH_OBJECT_TYPE *PPH_OBJECT_TYPE;

struct _PH_QUEUED_LOCK;
typedef struct _PH_QUEUED_LOCK PH_QUEUED_LOCK, *PPH_QUEUED_LOCK;

#ifdef DEBUG
typedef VOID (NTAPI *PPH_CREATE_OBJECT_HOOK)(
    _In_ PVOID Object,
    _In_ SIZE_T Size,
    _In_ ULONG Flags,
    _In_ PPH_OBJECT_TYPE ObjectType
    );
#endif

#ifndef _PH_REF_PRIVATE
extern PPH_OBJECT_TYPE PhObjectTypeObject;
extern PPH_OBJECT_TYPE PhAllocType;

#ifdef DEBUG
extern LIST_ENTRY PhDbgObjectListHead;
extern PH_QUEUED_LOCK PhDbgObjectListLock;
extern PPH_CREATE_OBJECT_HOOK PhDbgCreateObjectHook;
#endif
#endif

typedef struct _PH_OBJECT_TYPE_PARAMETERS
{
    SIZE_T FreeListSize;
    ULONG FreeListCount;

    UCHAR Reserved1;
    UCHAR Reserved2;
    UCHAR Reserved3;
    UCHAR Reserved4;
    ULONG Reserved5[4];
} PH_OBJECT_TYPE_PARAMETERS, *PPH_OBJECT_TYPE_PARAMETERS;

typedef struct _PH_OBJECT_TYPE_INFORMATION
{
    PWSTR Name;
    ULONG NumberOfObjects;
} PH_OBJECT_TYPE_INFORMATION, *PPH_OBJECT_TYPE_INFORMATION;

NTSTATUS PhInitializeRef(
    VOID
    );

_May_raise_
PHLIBAPI
NTSTATUS
NTAPI
PhCreateObject(
    _Out_ PVOID *Object,
    _In_ SIZE_T ObjectSize,
    _In_ ULONG Flags,
    _In_ PPH_OBJECT_TYPE ObjectType
    );

PHLIBAPI
VOID
NTAPI
PhReferenceObject(
    _In_ PVOID Object
    );

_May_raise_
PHLIBAPI
LONG
NTAPI
PhReferenceObjectEx(
    _In_ PVOID Object,
    _In_ LONG RefCount
    );

PHLIBAPI
BOOLEAN
NTAPI
PhReferenceObjectSafe(
    _In_ PVOID Object
    );

PHLIBAPI
VOID
NTAPI
PhDereferenceObject(
    _In_ PVOID Object
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDereferenceObjectDeferDelete(
    _In_ PVOID Object
    );

_May_raise_
PHLIBAPI
LONG
NTAPI
PhDereferenceObjectEx(
    _In_ PVOID Object,
    _In_ LONG RefCount,
    _In_ BOOLEAN DeferDelete
    );

PHLIBAPI
PPH_OBJECT_TYPE
NTAPI
PhGetObjectType(
    _In_ PVOID Object
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateObjectType(
    _Out_ PPH_OBJECT_TYPE *ObjectType,
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_opt_ PPH_TYPE_DELETE_PROCEDURE DeleteProcedure
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateObjectTypeEx(
    _Out_ PPH_OBJECT_TYPE *ObjectType,
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_opt_ PPH_TYPE_DELETE_PROCEDURE DeleteProcedure,
    _In_opt_ PPH_OBJECT_TYPE_PARAMETERS Parameters
    );

PHLIBAPI
VOID
NTAPI
PhGetObjectTypeInformation(
    _In_ PPH_OBJECT_TYPE ObjectType,
    _Out_ PPH_OBJECT_TYPE_INFORMATION Information
    );

FORCEINLINE VOID PhSwapReference(
    _Inout_ PVOID *ObjectReference,
    _In_opt_ PVOID NewObject
    )
{
    PVOID oldObject;

    oldObject = *ObjectReference;
    *ObjectReference = NewObject;

    if (NewObject) PhReferenceObject(NewObject);
    if (oldObject) PhDereferenceObject(oldObject);
}

FORCEINLINE VOID PhSwapReference2(
    _Inout_ PVOID *ObjectReference,
    _In_opt_ _Assume_refs_(1) PVOID NewObject
    )
{
    PVOID oldObject;

    oldObject = *ObjectReference;
    *ObjectReference = NewObject;

    if (oldObject) PhDereferenceObject(oldObject);
}

PHLIBAPI
NTSTATUS
NTAPI
PhCreateAlloc(
    _Out_ PVOID *Alloc,
    _In_ SIZE_T Size
    );

/** The size of the static array in an auto-release pool. */
#define PH_AUTO_POOL_STATIC_SIZE 64
/** The maximum size of the dynamic array for it to be
 * kept after the auto-release pool is drained. */
#define PH_AUTO_POOL_DYNAMIC_BIG_SIZE 256

/**
 * An auto-dereference pool can be used for
 * semi-automatic reference counting. Batches of
 * objects are dereferenced at a certain time.
 *
 * This object is not thread-safe and cannot
 * be used across thread boundaries. Always
 * store them as local variables.
 */
typedef struct _PH_AUTO_POOL
{
    ULONG StaticCount;
    PVOID StaticObjects[PH_AUTO_POOL_STATIC_SIZE];

    ULONG DynamicCount;
    ULONG DynamicAllocated;
    PVOID *DynamicObjects;

    struct _PH_AUTO_POOL *NextPool;
} PH_AUTO_POOL, *PPH_AUTO_POOL;

PHLIBAPI
VOID
NTAPI
PhInitializeAutoPool(
    _Out_ PPH_AUTO_POOL AutoPool
    );

_May_raise_
PHLIBAPI
VOID
NTAPI
PhDeleteAutoPool(
    _Inout_ PPH_AUTO_POOL AutoPool
    );

_May_raise_
PHLIBAPI
VOID
NTAPI
PhaDereferenceObject(
    _In_ PVOID Object
    );

PHLIBAPI
VOID
NTAPI
PhDrainAutoPool(
    _In_ PPH_AUTO_POOL AutoPool
    );

/**
 * Calls PhaDereferenceObject() and returns the given object.
 *
 * \param Object A pointer to an object. The value can be
 * null; in that case no action is performed.
 *
 * \return The value of \a Object.
 */
FORCEINLINE PVOID PHA_DEREFERENCE(
    _In_ PVOID Object
    )
{
    if (Object)
        PhaDereferenceObject(Object);

    return Object;
}

#ifdef __cplusplus
}
#endif

#endif
