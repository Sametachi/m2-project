add_library(YitsoraCore STATIC
	Constants/Controls.hpp
	Constants/Item.hpp
	Constants/NonPlayer.hpp
	Constants/Combat.hpp
	Constants/Group.hpp
	Constants/Guild.hpp
	Constants/Motion.hpp
	Constants/Skill.hpp
	Constants/Shop.hpp
	Net/PacketsCG.hpp
	Net/PacketsGC.hpp
	Net/PacketsGG.hpp
	Net/PacketsGD.hpp
	Net/PacketsDG.hpp
	Attribute.h
	Attribute.cpp
	Grid.cpp
	Grid.h
	GridPage.hpp
	Poly.cpp
	Poly.h
	ProtoReader.hpp
	ItemProtoReader.cpp
	MobProtoReader.cpp
	Tables.hpp
	Race/MotionProto.cpp
	Race/MotionProto.hpp
	Race/RaceConstans.hpp
	Race/RaceMotionConstants.cpp
	Race/RaceMotionConstants.hpp
	Race/EffectConstans.hpp
)

target_link_libraries(YitsoraCore PUBLIC YitsoraBasic)
set_target_properties(YitsoraCore PROPERTIES FOLDER "Libraries/Common")
