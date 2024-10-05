package main

import (
)

type DbObject interface {
}

/*
 * * ... varies based on type:
 *   * THING:
 *     * Home field as integer string
 *     * exit field as integer string
 *     * owner field as integer string
 *   * ROOM:
 *     * Drop to field as integer string
 *     * exit field as integer string
 *     * owner field as integer string
 *   * EXIT:
 *     * Number of exit destinations as integer string
 *     * The exit destinations, one per line, as integer strings
 *     * owner field as integer string
 *   * PLAYER:
 *     * Player home as integer string
 *     * exit field as integer string
 *     * player password
 *   * PROGRAM:
 *     * owner field as integer string
 */

// the fact that next is common to contents and exits lists implies
// that an object cannot be both an exit and a content --- an exit is
// never considered content under any circumstances
type DbObjectCommonFields struct {
	_ref DbRef
	_name string
	_location DbRef
	_head DbRef
	_next DbRef
	_flags int32
	_creationTime int32
	_lastUsedTime int32
	_useCount int32
	_modificationTime int32

	_propsList []string
	_isGarbage bool
}

type ObjectRoom struct {
	DbObjectCommonFields
	_dropto DbRef
	_head DbRef  // exits
	_owner DbRef
}

type ObjectThing struct {
	DbObjectCommonFields
	_home DbRef
	_head DbRef  // exits
	_owner DbRef
}

type ObjectExit struct {
	DbObjectCommonFields
	_numExits int64
	_exits []DbRef
	_owner DbRef
}

type ObjectPlayer struct {
	DbObjectCommonFields
	_home DbRef
	_head DbRef  // exits
	_password string
}

// this needs to reference another table
type ObjectProgram struct {
	DbObjectCommonFields
	_owner DbRef
}

const TYPE_ROOM    = 0x0
const TYPE_THING   = 0x1
const TYPE_EXIT    = 0x2
const TYPE_PLAYER  = 0x3
const TYPE_PROGRAM = 0x4
const TYPE_MASK    = 0x7

/*
#define WIZARD             0x10 **< gets automatic control *
#define LINK_OK            0x20 **< anybody can link to this *
#define DARK               0x40 **< contents of room are not printed *
#define INTERNAL           0x80 **< internal-use-only flag *
#define STICKY            0x100 **< this object goes home when dropped *
#define BUILDER           0x200 **< this player can use construction commands *
#define CHOWN_OK          0x400 **< this object can be \@chowned, or
                                 *   this player can see color
                                 *
#define JUMP_OK           0x800 **< A room which can be jumped from, or
                                 *   a player who can be jumped to
                                 *
#define KILL_OK          0x4000 **< Kill_OK bit.  Means you can be killed. *
#define GUEST            0x8000 **< Guest flag *
#define HAVEN           0x10000 **< can't kill here *
#define ABODE           0x20000 **< can set home here *
#define MUCKER          0x40000 **< programmer *
#define QUELL           0x80000 **< When set, wiz-perms are turned off *
#define SMUCKER        0x100000 **< second programmer bit.  For levels *
#define INTERACTIVE    0x200000 **< internal: player in MUF editor *
#define OBJECT_CHANGED 0x400000 **< internal: set when an object is dbdirty()ed *

#define VEHICLE       0x1000000 **< Vehicle flag *
#define ZOMBIE        0x2000000 **< Zombie flag *
#define LISTENER      0x4000000 **< internal: listener flag *
#define XFORCIBLE     0x8000000 **< externally forcible flag *
#define READMODE     0x10000000 **< internal: when set, player is in a READ *
#define SANEBIT      0x20000000 **< internal: used to check db sanity *
#define YIELD        0x40000000 **< Yield flag *
#define OVERT        0x80000000 **< Overt flag *
 */

