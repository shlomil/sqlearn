SQLearn
=======
A training tool for Databases/SQL course.
## SQLearn goals:
* SQL training environment.
* fully platform independant and light weight (Using QT).
* execute queries and view results instantly.
* save a "bundle" file of queries and database (homework file).
* As SQLite is sometimes too permissive we would like to prevent/alert user on : selection of non-aggregated fields in an aggregated query, double aggregated functions, nested selets in select parameter list. This requres implementing a simple SQL parser to detect these cases.

## TODO:
* delete query
* cancel rename on double click on tables
* double click to show table
* adding a label to table/queroes lists
* context menu on right click on query to delete it
* 'Window' menu to arrange windows.
* 'Query' menu for 'new query' / 'delete query option'
* 'about' menu.
* export option in file menu to export sqlearn file into separate sqlite db, and text files with queries
* new project discards unsaved project - fix it.
