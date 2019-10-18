add_library(skills
  src/skills.cpp
)

add_library(skills::skills ALIAS skills)

target_include_directories(skills PUBLIC include)

target_link_libraries(skills
  PRIVATE
    names::names
    pugixml
)
