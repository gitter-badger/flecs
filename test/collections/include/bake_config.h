/*
                                   )
                                  (.)
                                  .|.
                                  | |
                              _.--| |--._
                           .-';  ;`-'& ; `&.
                          \   &  ;    &   &_/
                           |"""---...---"""|
                           \ | | | | | | | /
                            `---.|.|.|.---'

 * This file is generated by bake.lang.c for your convenience. Headers of
 * dependencies will automatically show up in this file. Include bake_config.h
 * in your main project file. Do not edit! */

#ifndef COLLECTIONS_BAKE_CONFIG_H
#define COLLECTIONS_BAKE_CONFIG_H

/* Headers of public dependencies */
#include <flecs>
#include <bake.util>
#include <bake.test>

/* Headers of private dependencies */
#ifdef COLLECTIONS_IMPL
/* No dependencies */
#endif

/* Convenience macro for exporting symbols */
#if COLLECTIONS_IMPL && defined _MSC_VER
#define COLLECTIONS_EXPORT __declspec(dllexport)
#elif COLLECTIONS_IMPL
#define COLLECTIONS_EXPORT __attribute__((__visibility__("default")))
#elif defined _MSC_VER
#define COLLECTIONS_EXPORT __declspec(dllimport)
#else
#define COLLECTIONS_EXPORT
#endif

#endif

