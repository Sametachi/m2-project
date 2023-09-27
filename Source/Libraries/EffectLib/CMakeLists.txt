# GameLib Cmake
## TODO: do filters

set(FILES EffectData.cpp
EffectData.h
EffectElementBase.cpp
EffectElementBase.h
EffectElementBaseInstance.cpp
EffectElementBaseInstance.h
EffectInstance.cpp
EffectUpdateDecorator.cpp
EffectUpdateDecorator.h
EffectInstance.h
EffectManager.cpp
EffectManager.h
EffectMesh.cpp
EffectMesh.h
EffectMeshInstance.cpp
EffectMeshInstance.h
EmitterProperty.cpp
EmitterProperty.h
FrameController.cpp
FrameController.h
ParticleInstance.cpp
ParticleInstance.h
ParticleProperty.cpp
ParticleProperty.h
ParticleSystemData.cpp
ParticleSystemData.h
ParticleSystemInstance.cpp
ParticleSystemInstance.h
SimpleLightData.cpp
SimpleLightData.h
SimpleLightInstance.cpp
SimpleLightInstance.h
StdAfx.h
Type.cpp
Type.h)

add_library(EffectLib STATIC ${FILES})
target_link_libraries(EffectLib PUBLIC EterLib MilesLib)
set_target_properties(EffectLib PROPERTIES FOLDER Libraries/Client)
do_pch(PROJECT EffectLib)
