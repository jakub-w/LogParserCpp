project('LogParse', 'cpp',
	default_options : ['cpp_std=c++17'])

rapidjson_dep = dependency('RapidJSON')
mariadb_dep = dependency('mariadb')

# Qt
# qt5_mod = import('qt5')
# qt5_sql_dep = dependency('qt5', modules : ['Sql', 'Core'])
# End of Qt

# incl = include_directories('.', 'include')

executable('log-parse', dependencies : [rapidjson_dep, mariadb_dep,
					# qt5_sql_dep
				       ],
	   sources : ['main.cpp', 'LineParser.cpp',
		      'LineWriterFile.cpp', 'LineWriterDatabase.cpp',
		      # 'LineWriterQtDB.cpp', 'LineWriterSqlFile.cpp',
		      'LineWriterCout.cpp'],
	   # include_directories : incl
	  )
