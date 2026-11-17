/**
 * \file src/viewmodels/viewmodels.h
 * \brief ViewModel 层统一汇总头：Application 一次 include 即可引用全部 ViewModel。
 *
 * 包含顺序无依赖；各 ViewModel 之间不互相引用，全部通过模型层与信号协作。
 */

#ifndef VIEWMODELS_H
#define VIEWMODELS_H

#include "appviewmodel.h"
#include "charactersviewmodel.h"
#include "chatviewmodel.h"
#include "dataviewmodel.h"
#include "dialogservice.h"
#include "memoriesviewmodel.h"
#include "modelsettingsviewmodel.h"
#include "personasviewmodel.h"
#include "petviewmodel.h"
#include "recordsviewmodel.h"
#include "viewmodelbase.h"
#include "worldsviewmodel.h"

#endif // VIEWMODELS_H
