# Вызывается из POST_BUILD: копирует репозиторный data/ рядом с .exe только если там ещё нет accounts.txt.
# Иначе каждая пересборка затирала бы регистрации и правки администратора.

if(NOT DEFINED REPO_DATA_DIR OR NOT DEFINED EXE_OUTPUT_DIR)
    message(FATAL_ERROR "copy_initial_data_if_missing: задайте REPO_DATA_DIR и EXE_OUTPUT_DIR")
endif()

set(dest_data "${EXE_OUTPUT_DIR}/data")
set(marker "${dest_data}/accounts.txt")

if(EXISTS "${marker}")
    message(STATUS "Каталог data рядом с exe уже есть — не перезаписываем (сохраняются ваши данные).")
else()
    message(STATUS "Первый запуск: копируем шаблон data/ в ${dest_data}")
    file(MAKE_DIRECTORY "${dest_data}")
    file(COPY "${REPO_DATA_DIR}/." DESTINATION "${dest_data}")
endif()
