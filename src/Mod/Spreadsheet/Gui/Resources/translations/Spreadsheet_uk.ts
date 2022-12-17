<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="uk" sourcelanguage="en">
  <context>
    <name>Spreadsheet</name>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="739"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="748"/>
      <location filename="../../../App/Spreadsheet_legacy.py" line="874"/>
      <source>Cell</source>
      <translation>Комірка</translation>
    </message>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="751"/>
      <source>Apply</source>
      <translation>Застосувати</translation>
    </message>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="753"/>
      <source>Apply the changes to the current cell</source>
      <translation>Застосувати зміни до поточної комірки</translation>
    </message>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="755"/>
      <source>Delete</source>
      <translation>Видалити</translation>
    </message>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="757"/>
      <source>Deletes the contents of the current cell</source>
      <translation>Видаляє вміст поточної комірки</translation>
    </message>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="759"/>
      <source>Compute</source>
      <translation>Обчислити</translation>
    </message>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="761"/>
      <source>Updates the values handled by controllers</source>
      <translation>Оновлює значення, які обробляються елементами керування</translation>
    </message>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="907"/>
      <source>Create Spreadsheet</source>
      <translation>Створити таблицю</translation>
    </message>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="933"/>
      <source>Add controller</source>
      <translation>Додати елемент керування</translation>
    </message>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="959"/>
      <location filename="../../../App/Spreadsheet_legacy.py" line="973"/>
      <source>Add property controller</source>
      <translation>Додати контролер властивостей</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1179"/>
      <source>Normal</source>
      <translation>Звичайне</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1181"/>
      <source>Button</source>
      <translation>Кнопка</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1183"/>
      <source>ComboBox</source>
      <translation>ComboBox</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1185"/>
      <source>Label</source>
      <translation>Позначка</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1187"/>
      <source>Quantity</source>
      <translation>Кількість</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1189"/>
      <source>CheckBox</source>
      <translation>Відмітка</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1191"/>
      <source>Auto alias</source>
      <translation>Автоматичний псевдонім</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1193"/>
      <source>Auto alias vertical</source>
      <translation>Автоматичний псевдонім по вертикалі</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1195"/>
      <source>Color</source>
      <translation>Колір</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1206"/>
      <source>Reset edit mode</source>
      <translation type="unfinished">Reset edit mode</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1209"/>
      <source>Make a button with the current cell. Expects the cell to define a callable.
The button label is defined by the doc string of the callable. If empty,
then use the alias. If no alias, then use the cell address.</source>
      <translation type="unfinished">Make a button with the current cell. Expects the cell to define a callable.
The button label is defined by the doc string of the callable. If empty,
then use the alias. If no alias, then use the cell address.</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1214"/>
      <source>Edit the cell using a ComboBox. This mode Expects the cell to contain a 
list(dict, string), where the keys of dict defines the item list, and the
string defines the current item.

The cell also accepts list(list, int), where the inner list defines the item
list, and the int is the index of the current item.

In both caes, there can be a third optional item that defines a callable with
arguments (spreadsheet, cell_address, current_value, old_value). It will be
invoked after the user makes a new selection in the ComboBox.</source>
      <translation type="unfinished">Edit the cell using a ComboBox. This mode Expects the cell to contain a 
list(dict, string), where the keys of dict defines the item list, and the
string defines the current item.

The cell also accepts list(list, int), where the inner list defines the item
list, and the int is the index of the current item.

In both caes, there can be a third optional item that defines a callable with
arguments (spreadsheet, cell_address, current_value, old_value). It will be
invoked after the user makes a new selection in the ComboBox.</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1226"/>
      <source>Edit the cell using a plain text box. This edit mode is used to hide expression
details in the cell. The cell is expected to contain a list. And only the first
item will be shown, and the rest of items hidden

It can also be used to edit string property from other object using the double
binding function, e.g. dbind(Box.Label2).</source>
      <translation type="unfinished">Edit the cell using a plain text box. This edit mode is used to hide expression
details in the cell. The cell is expected to contain a list. And only the first
item will be shown, and the rest of items hidden

It can also be used to edit string property from other object using the double
binding function, e.g. dbind(Box.Label2).</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1234"/>
      <source>Edit the cell using a unit aware SpinBox. This mode expects the cell
to contain either a simple number, a &apos;quantity&apos; (i.e. number with unit)
or a list(quantity, dict). The dict contains optional keys (&apos;step&apos;,&apos;max&apos;,
&apos;min&apos;,&apos;unit&apos;). All keys are expects to have &apos;double&apos; type of value, except
&apos;unit&apos; which must be a string.

If no &apos;unit&apos; setting is found, the &apos;display unit&apos; setting of the current cell
will be used</source>
      <translation type="unfinished">Edit the cell using a unit aware SpinBox. This mode expects the cell
to contain either a simple number, a &apos;quantity&apos; (i.e. number with unit)
or a list(quantity, dict). The dict contains optional keys (&apos;step&apos;,&apos;max&apos;,
&apos;min&apos;,&apos;unit&apos;). All keys are expects to have &apos;double&apos; type of value, except
&apos;unit&apos; which must be a string.

If no &apos;unit&apos; setting is found, the &apos;display unit&apos; setting of the current cell
will be used</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1244"/>
      <source>Edit the cell using a CheckBox. The cell is expected to contain any value
that can be converted to boolean. If you want a check box with a title, use
a list(boolean, title).</source>
      <translation type="unfinished">Edit the cell using a CheckBox. The cell is expected to contain any value
that can be converted to boolean. If you want a check box with a title, use
a list(boolean, title).</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1249"/>
      <source>A pseudo edit mode that expects the content of the cell to be plain text.
It will use the first line of the text to set alias of the right sibling cell.
space is converted to &apos;_&apos;.

Moreover, a new cell added below an existing cell with &apos;Auto alias&apos; edit mode
will inherit this edit mode.</source>
      <translation type="unfinished">A pseudo edit mode that expects the content of the cell to be plain text.
It will use the first line of the text to set alias of the right sibling cell.
space is converted to &apos;_&apos;.

Moreover, a new cell added below an existing cell with &apos;Auto alias&apos; edit mode
will inherit this edit mode.</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1257"/>
      <source>Similar to &apos;Auto alias&apos; edit mode but works in vertical, i.e. assign alias to
the bottom sibling cell.</source>
      <translation type="unfinished">Similar to &apos;Auto alias&apos; edit mode but works in vertical, i.e. assign alias to
the bottom sibling cell.</translation>
    </message>
    <message>
      <location filename="../../../App/Cell.cpp" line="1261"/>
      <source>Edit the cell using a color button. The cell is expected to contain
a tuple of three or four floating numbers</source>
      <translation type="unfinished">Edit the cell using a color button. The cell is expected to contain
a tuple of three or four floating numbers</translation>
    </message>
  </context>
  <context>
    <name>Spreadsheet_Controller</name>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="919"/>
      <source>Add controller</source>
      <translation>Додати елемент керування</translation>
    </message>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="922"/>
      <source>Adds a cell controller to a selected spreadsheet</source>
      <translation>Додає елемент керування коміркою до вибраної електронної таблиці</translation>
    </message>
  </context>
  <context>
    <name>Spreadsheet_Create</name>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="902"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="905"/>
      <source>Adds a spreadsheet object to the active document</source>
      <translation>Додає об’єкт електронної таблиці до активного документа</translation>
    </message>
  </context>
  <context>
    <name>Spreadsheet_PropertyController</name>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="944"/>
      <source>Add property controller</source>
      <translation>Додати контролер властивостей</translation>
    </message>
    <message>
      <location filename="../../../App/Spreadsheet_legacy.py" line="947"/>
      <source>Adds a property controller to a selected spreadsheet</source>
      <translation>Додає контролер властивостей до вибраної електронної таблиці</translation>
    </message>
  </context>
  <context>
    <name>CmdCreateSpreadsheet</name>
    <message>
      <location filename="../../Command.cpp" line="887"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="888"/>
      <source>Create spreadsheet</source>
      <translation>Створити таблицю</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="889"/>
      <source>Create a new spreadsheet</source>
      <translation>Створити нову таблицю</translation>
    </message>
  </context>
  <context>
    <name>CmdSpreadsheetAlignBottom</name>
    <message>
      <location filename="../../Command.cpp" line="498"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="499"/>
      <source>Align bottom</source>
      <translation>Вирівняти по нижньому краю</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="500"/>
      <source>Bottom-align contents of selected cells</source>
      <translation>Вирівняти вміст виділених клітинок по нижньому краю</translation>
    </message>
  </context>
  <context>
    <name>CmdSpreadsheetAlignCenter</name>
    <message>
      <location filename="../../Command.cpp" line="342"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="343"/>
      <source>Align center</source>
      <translation>Вирівняти по центру</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="344"/>
      <source>Center-align contents of selected cells</source>
      <translation>Вирівняти по центру вміст виділених клітинок</translation>
    </message>
  </context>
  <context>
    <name>CmdSpreadsheetAlignLeft</name>
    <message>
      <location filename="../../Command.cpp" line="290"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="291"/>
      <source>Align left</source>
      <translation>Вирівняти по лівому краю</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="292"/>
      <source>Left-align contents of selected cells</source>
      <translation>Вирівняти ліворуч вміст виділених клітинок</translation>
    </message>
  </context>
  <context>
    <name>CmdSpreadsheetAlignRight</name>
    <message>
      <location filename="../../Command.cpp" line="394"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="395"/>
      <source>Align right</source>
      <translation>Вирівняти по правому краю</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="396"/>
      <source>Right-align contents of selected cells</source>
      <translation>Вирівняти праворуч вміст виділених клітинок</translation>
    </message>
  </context>
  <context>
    <name>CmdSpreadsheetAlignTop</name>
    <message>
      <location filename="../../Command.cpp" line="446"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="447"/>
      <source>Align top</source>
      <translation>Вирівняти по верхньому краю</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="448"/>
      <source>Top-align contents of selected cells</source>
      <translation>Вирівняти вміст виділених клітинок по верхньому краю</translation>
    </message>
  </context>
  <context>
    <name>CmdSpreadsheetAlignVCenter</name>
    <message>
      <location filename="../../Command.cpp" line="550"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="551"/>
      <source>Vertically center-align</source>
      <translation>Вертикальне вирівнювання по центру</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="552"/>
      <source>Vertically center-align contents of selected cells</source>
      <translation>Вирівнює вміст виділених комірок вертикально у центрі</translation>
    </message>
  </context>
  <context>
    <name>CmdSpreadsheetExport</name>
    <message>
      <location filename="../../Command.cpp" line="229"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="230"/>
      <source>Export spreadsheet</source>
      <translation>Експортувати таблицю</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="231"/>
      <source>Export spreadsheet to CSV file</source>
      <translation>Експортувати таблицю у CSV-файл</translation>
    </message>
  </context>
  <context>
    <name>CmdSpreadsheetImport</name>
    <message>
      <location filename="../../Command.cpp" line="179"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="180"/>
      <source>Import spreadsheet</source>
      <translation>Імпортувати таблицю</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="181"/>
      <source>Import CSV file into spreadsheet</source>
      <translation>Імпортувати CSV-файл у таблицю</translation>
    </message>
  </context>
  <context>
    <name>CmdSpreadsheetMergeCells</name>
    <message>
      <location filename="../../Command.cpp" line="70"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="71"/>
      <source>Merge cells</source>
      <translation>Об'єднати клітинки</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="72"/>
      <source>Merge selected cells</source>
      <translation>Об'єднати обрані комірки</translation>
    </message>
  </context>
  <context>
    <name>CmdSpreadsheetSetAlias</name>
    <message>
      <location filename="../../Command.cpp" line="823"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="824"/>
      <source>Set alias</source>
      <translation>Встановити псевдонім</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="825"/>
      <source>Set alias for selected cell</source>
      <translation>Встановити псевдонім для обраної комірки</translation>
    </message>
  </context>
  <context>
    <name>CmdSpreadsheetSplitCell</name>
    <message>
      <location filename="../../Command.cpp" line="123"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="124"/>
      <source>Split cell</source>
      <translation>Розділити клітинку</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="125"/>
      <source>Split previously merged cells</source>
      <translation>Розділити раніше обʼєднані комірки</translation>
    </message>
  </context>
  <context>
    <name>CmdSpreadsheetStyleBold</name>
    <message>
      <location filename="../../Command.cpp" line="602"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="603"/>
      <source>Bold text</source>
      <translation>Жирний текст</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="604"/>
      <source>Set text in selected cells bold</source>
      <translation>У виділених комірках відобразити текст жирним шрифтом</translation>
    </message>
  </context>
  <context>
    <name>CmdSpreadsheetStyleItalic</name>
    <message>
      <location filename="../../Command.cpp" line="676"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="677"/>
      <source>Italic text</source>
      <translation>Текст курсивом</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="678"/>
      <source>Set text in selected cells italic</source>
      <translation>У виділених комірках відобразити текст курсивом</translation>
    </message>
  </context>
  <context>
    <name>CmdSpreadsheetStyleUnderline</name>
    <message>
      <location filename="../../Command.cpp" line="750"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="751"/>
      <source>Underline text</source>
      <translation>Підкреслений текст</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="752"/>
      <source>Underline text in selected cells</source>
      <translation>У виділених комірках зробити текст підкресленим</translation>
    </message>
  </context>
  <context>
    <name>ColorPickerPopup</name>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="910"/>
      <source>Custom Color</source>
      <translation>Колір, який встановлюється користувачем</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="91"/>
      <source>Merge cells</source>
      <translation>Об'єднати клітинки</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="144"/>
      <source>Split cell</source>
      <translation>Розділити клітинку</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="312"/>
      <source>Left-align cell</source>
      <translation>Вирівнювання ліворуч</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="364"/>
      <source>Center cell</source>
      <translation>Вирівнювання по центру</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="416"/>
      <source>Right-align cell</source>
      <translation>Вирівнювання праворуч</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="468"/>
      <source>Top-align cell</source>
      <translation>Вирівнювання по верхньому краю</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="520"/>
      <source>Bottom-align cell</source>
      <translation>Вирівнювання по нижньому краю</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="572"/>
      <source>Vertically center cells</source>
      <translation>Вирівнювання вертикально по центру</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="641"/>
      <source>Set bold text</source>
      <translation>Зробити текст напівжирним</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="715"/>
      <source>Set italic text</source>
      <translation>Зробити текст курсивом</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="789"/>
      <source>Set underline text</source>
      <translation>Підкреслити текст</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="900"/>
      <source>Create Spreadsheet</source>
      <translation>Створити таблицю</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.cpp" line="238"/>
      <source>Set cell properties</source>
      <translation>Зміна властивостей комірок</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="361"/>
      <source>Remove cell alias</source>
      <translation>Видалити псевдонім для комірки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="394"/>
      <source>Cell edit mode</source>
      <translation>Режим редагування комірки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="416"/>
      <source>Cell persistent edit</source>
      <translation>Постійне редагування комірки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="435"/>
      <source>Recompute cells</source>
      <translation>Переобчислити комірки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="450"/>
      <source>Recompute cells only</source>
      <translation>Переобчислити лише комірки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="577"/>
      <location filename="../../SheetTableView.cpp" line="618"/>
      <source>Insert rows</source>
      <translation>Вставити рядки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="644"/>
      <source>Remove rows</source>
      <translation>Видалити рядки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="680"/>
      <location filename="../../SheetTableView.cpp" line="722"/>
      <source>Insert columns</source>
      <translation>Вставити стовпці</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="747"/>
      <source>Remove columns</source>
      <translation>Видалити стовпці</translation>
    </message>
    <message>
      <location filename="../../SpreadsheetView.cpp" line="143"/>
      <location filename="../../SheetTableView.cpp" line="951"/>
      <source>Clear cell(s)</source>
      <translation>Очистити комірку(и)</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="138"/>
      <source>Set foreground color</source>
      <translation>Вибір кольору переднього плану</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="165"/>
      <source>Set background color</source>
      <translation>Вибір кольору фону</translation>
    </message>
  </context>
  <context>
    <name>PropertiesDialog</name>
    <message>
      <location filename="../../PropertiesDialog.ui" line="14"/>
      <source>Cell properties</source>
      <translation>Властивості клітинки</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="37"/>
      <source>&amp;Color</source>
      <translation>&amp;Колір</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="43"/>
      <source>Text</source>
      <translation>Текст</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="66"/>
      <source>Background</source>
      <translation>Фон</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="90"/>
      <source>&amp;Alignment</source>
      <translation>&amp;Вирівнювання</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="98"/>
      <source>Horizontal</source>
      <translation>По горизонталі</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="105"/>
      <source>Left</source>
      <translation>Ліворуч</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="112"/>
      <location filename="../../PropertiesDialog.ui" line="156"/>
      <source>Center</source>
      <translation>Центр</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="119"/>
      <source>Right</source>
      <translation>Направо</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="142"/>
      <source>Vertical</source>
      <translation>По вертикалі</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="149"/>
      <source>Top</source>
      <translation>Згори</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="163"/>
      <source>Bottom</source>
      <translation>Внизу</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="189"/>
      <source>&amp;Style</source>
      <translation>&amp;Стиль</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="197"/>
      <source>Bold</source>
      <translation>Жирний</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="204"/>
      <source>Italic</source>
      <translation>Курсив</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="211"/>
      <source>Underline</source>
      <translation>Підкреслений</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="234"/>
      <source>&amp;Display unit</source>
      <translation>&amp;Одиниця відображення</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="256"/>
      <source>Unit string</source>
      <translation>Одиниця виміру</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="264"/>
      <source>A&amp;lias</source>
      <translation>П&amp;севдонім</translation>
    </message>
    <message>
      <location filename="../../PropertiesDialog.ui" line="273"/>
      <source>Alias for this cell</source>
      <translation>Псевдонім для цієї клітинки</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="191"/>
      <location filename="../../Command.cpp" line="247"/>
      <source>All (*)</source>
      <translation>Всі (*)</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="193"/>
      <source>Import file</source>
      <translation>Імпорт файлу</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="249"/>
      <source>Export file</source>
      <translation>Експорт файлу</translation>
    </message>
    <message>
      <location filename="../../ViewProviderSpreadsheet.cpp" line="145"/>
      <source>Show spreadsheet</source>
      <translation>Показати таблицю</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="84"/>
      <location filename="../../Workbench.cpp" line="86"/>
      <source>Set cell(s) foreground color</source>
      <translation>Встановити колір переднього плану комірки(ок)</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="85"/>
      <source>Sets the Spreadsheet cell(s) foreground color</source>
      <translation>Встановлює колір переднього плану комірки(ок) таблиці</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="97"/>
      <location filename="../../Workbench.cpp" line="99"/>
      <source>Set cell(s) background color</source>
      <translation>Встановити фоновий колір клітинки(ок)</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="98"/>
      <source>Sets the Spreadsheet cell(s) background color</source>
      <translation>Встановити фоновий колір клітинки(ок) таблиці</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="372"/>
      <source>Failed to remove alias</source>
      <translation>Не вдалося видалити псевдонім</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="410"/>
      <location filename="../../SheetTableView.cpp" line="429"/>
      <source>Failed to set edit mode</source>
      <translation>Не вдалося встановити режим редагування</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="444"/>
      <location filename="../../SheetTableView.cpp" line="458"/>
      <source>Failed to recompute cells</source>
      <translation>Не вдалося переобчислити комірки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="602"/>
      <source>Failed to insert rows</source>
      <translation>Не вдалося вставити рядки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="653"/>
      <source>Failed to remove rows</source>
      <translation>Не вдалося видалити рядки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="706"/>
      <source>Failed to insert columns</source>
      <translation>Не вдалося вставити стовпці</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="756"/>
      <source>Failed to remove columns</source>
      <translation>Не вдалося видалити стовпці</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="963"/>
      <source>Failed to clear cells</source>
      <translation>Не вдалося очистити комірки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="1101"/>
      <source>Copy &amp; Paste failed</source>
      <translation>Копіювання і вставлення провалено</translation>
    </message>
    <message>
      <location filename="../../SpreadsheetView.cpp" line="374"/>
      <source>Alias contains invalid characters!</source>
      <translation>Псевдонім містить некоректні символи!</translation>
    </message>
    <message>
      <location filename="../../SpreadsheetView.cpp" line="380"/>
      <source>Refer to cell by alias, for example
Spreadsheet.my_alias_name instead of Spreadsheet.B1</source>
      <translation>Зверніться до комірки за псевдонімом, наприклад
Spreadsheet.my_alias_name замість Spreadsheet.B1</translation>
    </message>
  </context>
  <context>
    <name>QtColorPicker</name>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="283"/>
      <location filename="../../qtcolorpicker.cpp" line="410"/>
      <location filename="../../qtcolorpicker.cpp" line="520"/>
      <source>Black</source>
      <translation>Чорний</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="411"/>
      <location filename="../../qtcolorpicker.cpp" line="521"/>
      <source>White</source>
      <translation>Білий</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="412"/>
      <location filename="../../qtcolorpicker.cpp" line="522"/>
      <source>Red</source>
      <translation>Червоний</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="413"/>
      <location filename="../../qtcolorpicker.cpp" line="523"/>
      <source>Dark red</source>
      <translation>Темно-червоний</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="414"/>
      <location filename="../../qtcolorpicker.cpp" line="524"/>
      <source>Green</source>
      <translation>Зелений</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="415"/>
      <location filename="../../qtcolorpicker.cpp" line="525"/>
      <source>Dark green</source>
      <translation>Темно-Зелений</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="416"/>
      <location filename="../../qtcolorpicker.cpp" line="526"/>
      <source>Blue</source>
      <translation>Cиній</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="417"/>
      <location filename="../../qtcolorpicker.cpp" line="527"/>
      <source>Dark blue</source>
      <translation>Темно-синій</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="418"/>
      <location filename="../../qtcolorpicker.cpp" line="528"/>
      <source>Cyan</source>
      <translation>Блакитний</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="419"/>
      <location filename="../../qtcolorpicker.cpp" line="529"/>
      <source>Dark cyan</source>
      <translation>Темно-блакитний</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="420"/>
      <location filename="../../qtcolorpicker.cpp" line="530"/>
      <source>Magenta</source>
      <translation>Пурпуровий</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="421"/>
      <location filename="../../qtcolorpicker.cpp" line="531"/>
      <source>Dark magenta</source>
      <translation>Темно-пурпуровий</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="422"/>
      <location filename="../../qtcolorpicker.cpp" line="532"/>
      <source>Yellow</source>
      <translation>Жовтий</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="423"/>
      <location filename="../../qtcolorpicker.cpp" line="533"/>
      <source>Dark yellow</source>
      <translation>Темно-жовтий</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="424"/>
      <location filename="../../qtcolorpicker.cpp" line="534"/>
      <source>Gray</source>
      <translation>Сірий</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="425"/>
      <location filename="../../qtcolorpicker.cpp" line="535"/>
      <source>Dark gray</source>
      <translation>Темно-сірий</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="426"/>
      <location filename="../../qtcolorpicker.cpp" line="536"/>
      <source>Light gray</source>
      <translation>Світло-сірий</translation>
    </message>
    <message>
      <location filename="../../qtcolorpicker.cpp" line="448"/>
      <source>Custom Color</source>
      <translation>Колір, який встановлюється користувачем</translation>
    </message>
  </context>
  <context>
    <name>Sheet</name>
    <message>
      <location filename="../../Sheet.ui" line="14"/>
      <source>Form</source>
      <translation>Форма</translation>
    </message>
    <message>
      <location filename="../../Sheet.ui" line="22"/>
      <source>&amp;Content:</source>
      <translation>&amp;Зміст:</translation>
    </message>
    <message>
      <location filename="../../Sheet.ui" line="39"/>
      <source>&amp;Alias:</source>
      <translation>&amp;Псевдонім:</translation>
    </message>
    <message>
      <location filename="../../Sheet.ui" line="52"/>
      <source>Refer to cell by alias, for example
Spreadsheet.my_alias_name instead of Spreadsheet.B1</source>
      <translation>Зверніться до комірки за псевдонімом, наприклад
Spreadsheet.my_alias_name замість Spreadsheet.B1</translation>
    </message>
  </context>
  <context>
    <name>SpreadsheetGui::DlgSettings</name>
    <message>
      <location filename="../../DlgSettings.ui" line="20"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../DlgSettings.ui" line="26"/>
      <source>Import/Export Settings</source>
      <translation>Параметри Імпорту/Експорту</translation>
    </message>
    <message>
      <location filename="../../DlgSettings.ui" line="45"/>
      <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Character used to delimit strings, typically is single quote (&apos;) or double quote (&amp;quot;). Must be a single character.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
      <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Символ, що використовується для розділення рядків, зазвичай це одинарні лапки (&apos;) або подвійні лапки (&amp;quot;). Має бути один символ.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="../../DlgSettings.ui" line="51"/>
      <source>&quot;</source>
      <translation>&quot;</translation>
    </message>
    <message>
      <location filename="../../DlgSettings.ui" line="70"/>
      <source>Delimiter Character: </source>
      <translation>Символ розділювача: </translation>
    </message>
    <message>
      <location filename="../../DlgSettings.ui" line="77"/>
      <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Escape character, typically the backslash (\), used to indicate special unprintable characters, e.g. \t = tab. Must be a single character.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
      <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Символ &quot;Escape&quot;, як правило, зворотний слеш (\), який використовується для позначення спеціальних символів, які не друкуються, напр. \t = вкладка. Має бути один символ.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="../../DlgSettings.ui" line="80"/>
      <source>\</source>
      <translation>\</translation>
    </message>
    <message>
      <location filename="../../DlgSettings.ui" line="38"/>
      <source>Escape Character: </source>
      <translation>Символ &quot;Escape&quot;: </translation>
    </message>
    <message>
      <location filename="../../DlgSettings.ui" line="106"/>
      <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Character to use as field delimiter.  Default is tab, but also commonly used are commas (,) and semicolons (;). Select from the list or enter your own in the field. Must be a single character or the words &lt;span style=&quot; font-style:italic;&quot;&gt;tab&lt;/span&gt;, &lt;span style=&quot; font-style:italic;&quot;&gt;comma&lt;/span&gt;, or &lt;span style=&quot; font-style:italic;&quot;&gt;semicolon&lt;/span&gt;.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
      <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Символ для позначення роздільника полів. За замовчуванням це символ табулювання, але також часто використовуються коми (,) і крапка з комою (;). Виберіть зі списку або введіть свій власний у поле. Має бути один символ або слова &lt;span style=&quot; font-style:italic;&quot;&gt;табуляція&lt;/span&gt;, &lt;span style=&quot; font-style:italic;&quot;&gt;кома&lt;/span&gt;, або &lt;span style=&quot; font-style:italic;&quot;&gt;крапка з комою&lt;/span&gt;.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="../../DlgSettings.ui" line="122"/>
      <source>tab</source>
      <translation>табуляція</translation>
    </message>
    <message>
      <location filename="../../DlgSettings.ui" line="127"/>
      <source>;</source>
      <translation>;</translation>
    </message>
    <message>
      <location filename="../../DlgSettings.ui" line="132"/>
      <source>,</source>
      <translation>,</translation>
    </message>
    <message>
      <location filename="../../DlgSettings.ui" line="99"/>
      <source>Quote Character: </source>
      <translation>Символ лапок: </translation>
    </message>
  </context>
  <context>
    <name>SpreadsheetGui::SheetTableView</name>
    <message>
      <location filename="../../SheetTableView.cpp" line="206"/>
      <source>Properties...</source>
      <translation>Властивості...</translation>
    </message>
    <message numerus="yes">
      <location filename="../../SheetTableView.cpp" line="143"/>
      <source>Insert %n row(s) above</source>
      <translation>
        <numerusform>Вставити %n рядок(ів) вище</numerusform>
        <numerusform>Вставити %n рядків вище</numerusform>
        <numerusform>Вставити %n рядків вище</numerusform>
        <numerusform>Вставити %n рядок(ків) вище</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="117"/>
      <source>Show all rows</source>
      <translation>Показати всі рядки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="120"/>
      <source>Show all columns</source>
      <translation>Показати всі стовпці</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="131"/>
      <source>Bind...</source>
      <translation>Прив’язати...</translation>
    </message>
    <message numerus="yes">
      <location filename="../../SheetTableView.cpp" line="147"/>
      <source>Insert %n row(s) below</source>
      <translation>
        <numerusform>Вставити %n рядок нижче</numerusform>
        <numerusform>Вставити %n рядків нижче</numerusform>
        <numerusform>Вставити %n рядків нижче</numerusform>
        <numerusform>Вставити %n рядок(ків) нижче</numerusform>
      </translation>
    </message>
    <message numerus="yes">
      <location filename="../../SheetTableView.cpp" line="151"/>
      <source>Insert %n non-contiguous rows</source>
      <translation>
        <numerusform>Вставити %n несуміжний рядок</numerusform>
        <numerusform>Вставити %n несуміжних рядків</numerusform>
        <numerusform>Вставити %n несуміжних рядків</numerusform>
        <numerusform>Вставити %n несуміжний(х) рядок(ів)</numerusform>
      </translation>
    </message>
    <message numerus="yes">
      <location filename="../../SheetTableView.cpp" line="154"/>
      <source>Remove row(s)</source>
      <translation>
        <numerusform>Видалити рядок</numerusform>
        <numerusform>Видалити декілька рядків</numerusform>
        <numerusform>Видалити рядки</numerusform>
        <numerusform>Видалити рядок(ки)</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="159"/>
      <source>Toggle row visibility</source>
      <translation>Увімкнути видимість рядка</translation>
    </message>
    <message numerus="yes">
      <location filename="../../SheetTableView.cpp" line="178"/>
      <source>Insert %n column(s) left</source>
      <translation>
        <numerusform>Вставити %n стовпець ліворуч</numerusform>
        <numerusform>Вставити %n стовпця ліворуч</numerusform>
        <numerusform>Вставити %n стовпців ліворуч</numerusform>
        <numerusform>Вставити %n стовпець(і) ліворуч</numerusform>
      </translation>
    </message>
    <message numerus="yes">
      <location filename="../../SheetTableView.cpp" line="182"/>
      <source>Insert %n column(s) right</source>
      <translation>
        <numerusform>Вставити %n стовпець праворуч</numerusform>
        <numerusform>Вставити %n стовпця праворуч</numerusform>
        <numerusform>Вставити %n стовпців праворуч</numerusform>
        <numerusform>Вставити %n стовпець(і) праворуч</numerusform>
      </translation>
    </message>
    <message numerus="yes">
      <location filename="../../SheetTableView.cpp" line="186"/>
      <source>Insert %n non-contiguous columns</source>
      <translation>
        <numerusform>Вставити %n несуміжний стовпець</numerusform>
        <numerusform>Вставити %n несуміжних стовпців</numerusform>
        <numerusform>Вставити %n несуміжних стовпців</numerusform>
        <numerusform>Вставити %n несуміжний(х) стовпець(ів)</numerusform>
      </translation>
    </message>
    <message numerus="yes">
      <location filename="../../SheetTableView.cpp" line="189"/>
      <source>Remove column(s)</source>
      <translation>
        <numerusform>Видалити стовпець</numerusform>
        <numerusform>Видалити стовпці</numerusform>
        <numerusform>Видалити стовпці</numerusform>
        <numerusform>Видалити стовпець(і)</numerusform>
      </translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="194"/>
      <source>Toggle column visibility</source>
      <translation>Перемкнути видимість стовпця</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="210"/>
      <source>Alias...</source>
      <translation>Псевдонім...</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="214"/>
      <source>Remove alias(es)</source>
      <translation>Видалити псевдонім(и)</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="282"/>
      <source>Edit mode</source>
      <translation>Режим редагування</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="289"/>
      <source>Persistent</source>
      <translation>Постійно</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="296"/>
      <source>Recompute cells</source>
      <translation>Переобчислити комірки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="299"/>
      <source>Mark selected cells as touched, and recompute the entire spreadsheet</source>
      <translation>Позначити вибрані комірки як редаговані та перерахувати всю таблицю</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="301"/>
      <source>Recompute cells only</source>
      <translation>Переобчислити лише комірки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="304"/>
      <source>Recompute only the selected cells without touching other depending cells
It can be used as a way out of tricky cyclic dependency problem, but may
may affect cells dependency coherence. Use with care!</source>
      <translation>Повторно обчислює лише вибрані клітинки, не торкаючись інших залежних комірок. 
Це можна використовувати як вирішення проблеми циклічної залежності, але може 
вплинути на узгодженість залежностей комірок. Використовуйте з обережністю!</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="312"/>
      <source>Configuration table...</source>
      <translation>Таблиця Конфігурації...</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="317"/>
      <source>Merge cells</source>
      <translation>Об’єднати комірки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="319"/>
      <source>Split cells</source>
      <translation>Роз’єднати комірки</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="322"/>
      <source>Cut</source>
      <translation>Вирізати</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="324"/>
      <source>Delete</source>
      <translation>Видалити</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="326"/>
      <source>Copy</source>
      <translation>Копіювати</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="328"/>
      <source>Paste</source>
      <translation>Вставити</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="331"/>
      <source>Paste special...</source>
      <translation>Спеціальна вставка...</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="333"/>
      <source>Paste value</source>
      <translation>Вставити значення</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="335"/>
      <source>Paste format</source>
      <translation>Вставити формат</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="337"/>
      <source>Paste value &amp;&amp; format</source>
      <translation>Вставити значення &amp;&amp; формат</translation>
    </message>
    <message>
      <location filename="../../SheetTableView.cpp" line="339"/>
      <source>Paste formula</source>
      <translation>Вставити формулу</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Spreadsheet</source>
      <translation>Таблиця</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>&amp;Spreadsheet</source>
      <translation>&amp;Таблиця</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="51"/>
      <source>&amp;Alignment</source>
      <translation>&amp;Вирівнювання</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="52"/>
      <source>&amp;Styles</source>
      <translation>&amp;Стилі</translation>
    </message>
  </context>
  <context>
    <name>DlgBindSheet</name>
    <message>
      <location filename="../../DlgBindSheet.ui" line="14"/>
      <source>Bind Spreadsheet Cells</source>
      <translation>Зв’язування комірок таблиці</translation>
    </message>
    <message>
      <location filename="../../DlgBindSheet.ui" line="20"/>
      <source>From cells:</source>
      <translation>З комірки:</translation>
    </message>
    <message>
      <location filename="../../DlgBindSheet.ui" line="27"/>
      <source>Binding cell range start</source>
      <translation>Початковий діапазон зв’язування комірок</translation>
    </message>
    <message>
      <location filename="../../DlgBindSheet.ui" line="34"/>
      <source>Binding cell range end
</source>
      <translation>Кінцевий діапазон зв’язування комірок
</translation>
    </message>
    <message>
      <location filename="../../DlgBindSheet.ui" line="42"/>
      <source>To cells:</source>
      <translation>До комірки:</translation>
    </message>
    <message>
      <location filename="../../DlgBindSheet.ui" line="49"/>
      <source>Starting cell address to bind to. Type &apos;=&apos; if you want to use expression.
The expression must evaluates to a string of some cell address.</source>
      <translation>Початковий діапазон для зв’язування комірок. Наберіть &apos;=&apos; якщо хочете використати вираз.
Вираз має бути строкою з використанням адреси комірки.</translation>
    </message>
    <message>
      <location filename="../../DlgBindSheet.ui" line="57"/>
      <source>Ending cell address to bind to. Type &apos;=&apos; if you want to use expression.
The expression must evaluates to a string of some cell address.</source>
      <translation>Кінцевий діапазон для зв’язування комірок. Наберіть &apos;=&apos; якщо хочете використати вираз.
Вираз має бути строкою з використанням адреси комірки.</translation>
    </message>
    <message>
      <location filename="../../DlgBindSheet.ui" line="65"/>
      <source>Sheet:</source>
      <translation>Таблиця:</translation>
    </message>
    <message>
      <location filename="../../DlgBindSheet.ui" line="72"/>
      <source>Use hidden reference not avoid creating a depdenecy with the referenced object. Use with caution!</source>
      <translation>Використання прихованого посилання дозволяє уникнути створення залежності з об’єктом, на який посилаються. Використовуйте з обережністю!</translation>
    </message>
    <message>
      <location filename="../../DlgBindSheet.ui" line="75"/>
      <source>Use hidden reference</source>
      <translation>Використати приховані посилання</translation>
    </message>
    <message>
      <location filename="../../DlgBindSheet.ui" line="84"/>
      <source>Unbind</source>
      <translation>Відв’язати</translation>
    </message>
    <message>
      <location filename="../../DlgBindSheet.ui" line="91"/>
      <source>Cancel</source>
      <translation>Відмінити</translation>
    </message>
    <message>
      <location filename="../../DlgBindSheet.ui" line="98"/>
      <source>OK</source>
      <translation>Зв’язати</translation>
    </message>
    <message>
      <location filename="../../DlgBindSheet.ui" line="110"/>
      <source>Select which spread sheet to bind to.</source>
      <translation>Виберіть таблицю для зв’язування.</translation>
    </message>
  </context>
  <context>
    <name>DlgSheetConf</name>
    <message>
      <location filename="../../DlgSheetConf.ui" line="14"/>
      <source>Setup Configuration Table</source>
      <translation>Створення Таблиці Конфігурацій</translation>
    </message>
    <message>
      <location filename="../../DlgSheetConf.ui" line="20"/>
      <source>Property:</source>
      <translation>Властивість:</translation>
    </message>
    <message>
      <location filename="../../DlgSheetConf.ui" line="27"/>
      <source>Ending cell address.

The first column of the range is assumed to contain a list of configuration
names, which will be used to generate a string list and bind to the given
property for user to dynamically switch configuration.

The first row of the range will be bound to whatever row (indirectly) selected
by that property.
</source>
      <translation>Кінцева адреса комірки.

Передбачається, що перший стовпець діапазону містить список імен конфігурації,
які будуть використовуватися для створення списку рядків та прив’язки до цієї 
властивості, щоб користувач міг динамічно перемикати конфігурації.

Перший рядок діапазону буде прив’язаний до будь-якого рядка (непрямо), 
обраного за допомогою цієї властивості.
</translation>
    </message>
    <message>
      <location filename="../../DlgSheetConf.ui" line="42"/>
      <source>Cell range:</source>
      <translation>Діап. комірок:</translation>
    </message>
    <message>
      <location filename="../../DlgSheetConf.ui" line="49"/>
      <source>Starting cell address.

The first column of the range is assumed to contain a list of configuration
names, which will be used to generate a string list and bind to the given
property for user to dynamically switch configuration.

The first row of the range will be bound to whatever row (indirectly) selected
by that property.
</source>
      <translation>Початкова адреса комірки.

Передбачається, що перший стовпець діапазону містить список імен конфігурації,
які будуть використовуватися для створення списку рядків та прив’язки до цієї 
властивості, щоб користувач міг динамічно перемикати конфігурації.

Перший рядок діапазону буде прив’язаний до будь-якого рядка (непрямо), 
обраного за допомогою цієї властивості.
</translation>
    </message>
    <message>
      <location filename="../../DlgSheetConf.ui" line="64"/>
      <source>Type in an expression to specify the object and property name to dynamically
switch the design configuration. The property will be created if not exist.</source>
      <translation>Введіть вираз, щоб вказати назву об’єкта та властивості, щоб динамічно 
змінювати конфігурацію проекту. Якщо властивість не існує, вона буде створена.</translation>
    </message>
    <message>
      <location filename="../../DlgSheetConf.ui" line="72"/>
      <source>Optional property group name.</source>
      <translation>Не обов’язкова назва групи властивостей.</translation>
    </message>
    <message>
      <location filename="../../DlgSheetConf.ui" line="79"/>
      <source>Group:</source>
      <translation>Група:</translation>
    </message>
    <message>
      <location filename="../../DlgSheetConf.ui" line="88"/>
      <source>Unsetup</source>
      <translation>Видалити</translation>
    </message>
    <message>
      <location filename="../../DlgSheetConf.ui" line="95"/>
      <source>Cancel</source>
      <translation>Відмінити</translation>
    </message>
    <message>
      <location filename="../../DlgSheetConf.ui" line="102"/>
      <source>OK</source>
      <translation>Зв’язати</translation>
    </message>
  </context>
  <context>
    <name>SpreadsheetGui::SheetModel</name>
    <message>
      <location filename="../../SheetModel.cpp" line="559"/>
      <source>Edit cell</source>
      <translation>Редагування комірки</translation>
    </message>
  </context>
  <context>
    <name>SpreadsheetGui::SheetView</name>
    <message>
      <location filename="../../SpreadsheetView.cpp" line="334"/>
      <source>Edit alias</source>
      <translation>Редагувати псевдонім</translation>
    </message>
    <message>
      <location filename="../../SpreadsheetView.cpp" line="334"/>
      <source>Remove alias</source>
      <translation>Видалити псевдонім</translation>
    </message>
    <message>
      <location filename="../../SpreadsheetView.cpp" line="348"/>
      <source>Edit cell alias</source>
      <translation>Редагувати псевдонім комірки</translation>
    </message>
    <message>
      <location filename="../../SpreadsheetView.cpp" line="349"/>
      <source>Failed to edit cell alias: %1</source>
      <translation>Не вдалося відредагувати псевдонім комірки: %1</translation>
    </message>
  </context>
  <context>
    <name>SpreadsheetGui::DlgSheetConf</name>
    <message>
      <location filename="../../DlgSheetConf.cpp" line="246"/>
      <source>Setup configuration table</source>
      <translation>Створити Таблицю Конфігурацій</translation>
    </message>
    <message>
      <location filename="../../DlgSheetConf.cpp" line="293"/>
      <source>Unsetup configuration table</source>
      <translation>Видалити таблицю конфігурацій</translation>
    </message>
  </context>
  <context>
    <name>SpreadsheetGui::DlgBindSheet</name>
    <message>
      <location filename="../../DlgBindSheet.cpp" line="192"/>
      <location filename="../../DlgBindSheet.cpp" line="220"/>
      <source>Bind cells</source>
      <translation>Зв’язати комірки</translation>
    </message>
    <message>
      <location filename="../../DlgBindSheet.cpp" line="193"/>
      <source>Source and target cell count mismatch. Partial binding may still work.

Do you want to continue?</source>
      <translation type="unfinished">Source and target cell count mismatch. Partial binding may still work.

Do you want to continue?</translation>
    </message>
    <message>
      <location filename="../../DlgBindSheet.cpp" line="238"/>
      <source>Unbind cells</source>
      <translation>Відв’язати комірки</translation>
    </message>
  </context>
  <context>
    <name>Py</name>
    <message>
      <location filename="../../AppSpreadsheetGui.cpp" line="87"/>
      <source>Unnamed</source>
      <translation>Без назви</translation>
    </message>
  </context>
</TS>
