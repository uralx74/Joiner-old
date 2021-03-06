#include "pch.h"
#pragma hdrstop

#include "StorageOra.h"


//---------------------------------------------------------------------------
// TStorageOra
//---------------------------------------------------------------------------
// �������� ��������� ������ � TOraSession � TOraQuery
//  try {
//      OraSession->StartTransaction();
//      OraQuery->Add("sql text"); ���  OraQuery->CreateProcCall("sql text",0);
//      OraQuery->FieldByName("field")->Value = ... ��� OraQuery->ParamByName("field")->Value = ...
//      OraQuery->ExecSQL();
//      OraSession->Commit();
//  } catch (Exception &e) {
//      OraSession->Rollback();
//      Application->ShowException(&e);
//  }
//
//
//---------------------------------------------------------------------------
//

TStorageOra::TStorageOra() :
    dbSession(NULL),
    dbQuery(NULL)
{
}


void TStorageOra::openConnection(AnsiString Server, AnsiString Username, AnsiString Password)
{
    try {
        // ��������� ��������
        dbSession = new TOraSession(NULL);
        //ThreadOraSession->OnError = OraSession1Error;
        dbSession->LoginPrompt = false;
        dbSession->Password = Password;
        dbSession->Username = Username;
        dbSession->Server = Server;
        //dbSession->Options = TemplateOraSession->Options;
        //dbSession->HomeName = TemplateOraSession->HomeName;
        dbSession->Options->Direct = true;
        dbSession->ConnectMode = cmNormal;
        dbSession->Pooling = false;
        dbSession->ThreadSafety = true;
        dbSession->AutoCommit = false;
        dbSession->DataTypeMap->Clear();
        dbSession->Connect();
    }
    catch (Exception& e)
    {
        delete dbSession;
        dbSession = NULL;
        throw Exception("Can't connect to server \"" + Server + "\". " + Trim(e.Message) );
    }



 /*   TOraQuery *OraQuery = new TOraQuery(NULL);
    try
    {
        OraQuery->Session = dbSession;
        OraQuery->SQL->Clear();
        OraQuery->SQL->Add("ALTER SESSION SET NLS_LANGUAGE = RUSSIAN");
        OraQuery->Execute();
        OraQuery->SQL->Clear();
        OraQuery->SQL->Add("ALTER SESSION SET NLS_TERRITORY = AMERICA");
        OraQuery->Execute();
        OraQuery->SQL->Clear();
        OraQuery->SQL->Add("ALTER SESSION SET NLS_DATE_FORMAT = 'DD.MM.YYYY'");
        OraQuery->Execute();
        CurrencyString = "�.";
        DecimalSeparator = '.';
        CurrencyDecimals = 2;
        ThousandSeparator = ' ';
        ShortDateFormat = "dd.mm.yyyy";
        DateSeparator = '.';
        OraQuery->SQL->Clear();
        OraQuery->SQL->Add("alter session set nls_numeric_characters='"+ AnsiString(DecimalSeparator)+AnsiString(ThousandSeparator)+"'");
        OraQuery->Execute();
        OraQuery->Close();
        OraQuery->SQL->Clear();
        delete OraQuery;
    } catch (Exception &e) {
        throw e;
    }  */

}

//---------------------------------------------------------------------------
//
void TStorageOra::closeTable()
{
    if (Active)
    {
        if (dbQuery->Active)
        {
            dbQuery->Close();
        }
        dbSession->Close();
    }

    if (dbQuery != NULL)
    {
        delete dbQuery;
        dbQuery = NULL;
    }

    if (dbSession != NULL)
    {
        delete dbSession;
        dbSession = NULL;
    }

    TStorage::closeTable();
}

//---------------------------------------------------------------------------
//
TStorageOra::~TStorageOra()
{
    TStorageOra::closeTable();
}

//---------------------------------------------------------------------------
//
void TStorageOra::prepareQuery()
{
    try
    {
   	    dbQuery = new TOraQuery(NULL);
        dbQuery->Session = dbSession;
        dbQuery->FetchAll = true;
        dbQuery->AutoCommit = false;

        //dbQuery->CachedUpdates = true;
        dbQuery->SQL->Clear();

    }
    catch (...)
    {
        delete dbQuery;
        dbQuery = NULL;
        throw Exception("");
    }
}

//---------------------------------------------------------------------------
// ������� �������
void TStorageOra::truncateTable(TStorageTableOra* Table)
{
    if (!ReadOnly && Table->Truncate && Table->Table != "")
    {
        try
        {
            //dbQuery->SQL->Add("delete from " + Table->Table);
            dbQuery->SQL->Add("truncate table " + Table->Table);
            dbQuery->ExecSQL();
            dbQuery->SQL->Clear();
        } catch (...) {
            delete dbQuery;
            dbQuery = NULL;
            throw Exception("Can't truncate table " + Table->Table + ".");
        }
    }
    else
    {
        throw Exception("Can't truncate ReadOnly table .");
    }
}

/* ���������� ���������������� ��������� */
void TStorageOra::executeProc(const String& procName)
{
    try
    {
        dbQuery->CreateProcCall(procName, 0);
        dbQuery->ExecSQL();
    }
    catch(Exception &e)
    {
        delete dbQuery;
        dbQuery = NULL;
        throw Exception("Procedure \"" + procName + "\" return exception message: \"" + e.Message + "\"");
    }
}


//---------------------------------------------------------------------------
// �������� ������������ OraQuery
void TStorageOraProc::openTable(bool ReadOnly)
{
    this->ReadOnly = ReadOnly;

    /*�������� ��������������� TStorageOra::Open*/

    if (Tables.size()>0)
    {
        openConnection(Tables[TableIndex].Server, Tables[TableIndex].Username, Tables[TableIndex].Password);
    }
    else
    {
        throw Exception("Storage is not specified.");
    }

    prepareQuery();

    if(ReadOnly == false && Tables[TableIndex].InitProcName != "" )  // ��������� ���������������� ��������� ���� ��� ������
    {
        executeProc(Tables[TableIndex].InitProcName);
    }

    if (ReadOnly == false && Tables[TableIndex].Table != "" && Tables[TableIndex].Truncate == true)
    {
        truncateTable(&Tables[TableIndex]);
    }

    try
    {
 	    dbQuery->CreateProcCall(Tables[TableIndex].Procedure, 0);
    }
    catch (...)
    {
        delete dbQuery;
        dbQuery = NULL;
        throw Exception("Can't create procedure call.");
    }

    FieldCount = Fields.size();
    Active = true;
}

//---------------------------------------------------------------------------
// ������������� �������� ��������� ����
void TStorageOraProc::setFieldValue(Variant Value)
{
    if (Fields[FieldIndex]->active && Fields[FieldIndex]->enable)
    {
        dbQuery->ParamByName(Fields[FieldIndex]->name)->Value = Value;
    }
}

//---------------------------------------------------------------------------
// ���������� ���������
void TStorageOraProc::post()
{
    dbQuery->ExecSQL();
    FieldIndex = 0;
    //RecordCount++;  2016-11-09
    Modified = true;
}


//---------------------------------------------------------------------------
//
void TStorageOraProc::nextField()
{
    if (!eof())
    {
        FieldIndex++;
    }
}

//---------------------------------------------------------------------------
// ���������� ��������������� ��������� ���� - ��� ���� � �������-���������
/*AnsiString TStorageOraProc::GetSrcField()
{
    return Fields[FieldIndex]->name_src;
}*/

//---------------------------------------------------------------------------
//
/*bool TStorageOraProc::IsActiveField()
{
    return Fields[FieldIndex]->active && Fields[FieldIndex]->enable;
} */

//---------------------------------------------------------------------------
//
TOraField* TStorageOraProc::addField()
{
    TOraField* Field = new TOraField();
    Fields.push_back(dynamic_cast<TStorageField*>(Field));
    FieldCount++;
    return Field;

}

/*
//---------------------------------------------------------------------------
//
void TStorageOraProc::AddField(const TOraField& Field)
{
    Fields.push_back(Field);
    FieldCount++;
} */

//---------------------------------------------------------------------------
//
void TStorageOraProc::addTable(const TOraProcTable& Table)
{
    Tables.push_back(Table);
    TableCount++;
}

//---------------------------------------------------------------------------
// ���������� ������������ ��������� ���������/��������� ������
AnsiString TStorageOraProc::getTable()
{
    //return Procedure;
    if (!eot()) {
        if (Tables[TableIndex].Table != "") {   // ���� ������ �������, �� ���������� ��� �������
            return Tables[TableIndex].Table;
        } else {
            return Tables[TableIndex].Procedure;
        }
    }
}

//---------------------------------------------------------------------------
//  TStorageOraSql
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
void TStorageOraSql::openTable(bool ReadOnly)
{
    this->ReadOnly = ReadOnly;

    if ( Tables.size() > 0 )
    {
        int retry_count = Tables[TableIndex].retry_count;
        int retry_interval = Tables[TableIndex].retry_interval * 1000;
        int retry_number = 1;
        bool connected = false;
        while ( !connected && ( retry_number <= retry_count || retry_count == -1 ) )
        {
            try
            {
                if ( retry_count != 1 )
                {
                    Logger->WriteLog("������� ���������� " + IntToStr(retry_number));
                }
                openConnection(Tables[TableIndex].Server, Tables[TableIndex].Username, Tables[TableIndex].Password);
                connected = true;
            }
            catch(Exception &e)
            {
                retry_number++;
                if (retry_number > retry_count && retry_count != -1)
                {
                    throw Exception(e);
                }
                else
                {
                    // ����� � ���
                    Logger->WriteLog(e.Message);
                    Logger->WriteLog("������� ���������� �� �������. �������� " + IntToStr(retry_interval) + " ���." );
                    Sleep(retry_interval);
                }
            }
        }
    }
    else
    {
        throw Exception("Storage is not specified.");
    }

    prepareQuery();

    if(ReadOnly == false && Tables[TableIndex].InitProcName != "" )  // ��������� ���������������� ��������� ���� ��� ������
    {
        executeProc(Tables[TableIndex].InitProcName);
    }


    if (ReadOnly == false && Tables[TableIndex].Truncate)  // ������� �������
    {
        truncateTable(&Tables[TableIndex]);
    }

    AnsiString Sql = Tables[TableIndex].Sql;
    AnsiString SqlText;

    if (Sql != "") {
        if (!FileExists(Sql))
        {
            throw Exception("File not found " + Sql + ".");
        }
        TStringList* pStringList;
        pStringList = new TStringList();
        pStringList->Clear();
        pStringList->LoadFromFile(Sql);
        SqlText = pStringList->Text;
        pStringList->Free();
    } else {
        SqlText =  "select * from " + Tables[TableIndex].Table + (ReadOnly == true? "": " for update");
    }

    dbQuery->SQL->Clear();
    dbQuery->SQL->Add(SqlText);

    try {
        dbQuery->Open();
        RecordCount = dbQuery->RecordCount;
    } catch (...) {
        throw Exception("Can't to open query.");
    }

    // �������� ������� �������� ���� - ������� ����������
    // �������� ��� ��������.
    // ���� ��������, �� ����������� �������� FieldsCount = dbQuery->Fields->Count
    //FieldCount = dbQuery->Fields->Count;
    // �������� ������� ���� �� OraQuery->Fields � ��������� Fields
    // ����  Fields.size() == 0
    FieldCount = Fields.size();
    if (FieldCount == 0) {
        FieldCount = dbQuery->Fields->Count;

        TStringList *fieldList = new TStringList();
        dbQuery->Fields->GetFieldNames(fieldList);

        for (int i = 0; i < dbQuery->Fields->Count; i++) {
            TOraField* field = addField();      // �������� �������� ��
            field->name = LowerCase(fieldList->Strings[i]);
            field->active = true;
            field->enable = true;
            field->name_src = field->name;
        }
        fieldList->Free();
    }

    //dbSession->StartTransaction();
    Active = true;
}

//---------------------------------------------------------------------------
// ���������� ������� ������� ���� � ������
//bool TStorageOraSql::FindField(AnsiString fieldName)
//{
//    return dbQuery->Fields->FindField(fieldName);
//}

//---------------------------------------------------------------------------
// ���������� �������� ����
//Variant TStorageOraSql::Get(AnsiString Field)
Variant TStorageOraSql::getFieldValue(TStorageField* Field)
{
    // �������� ����������?????? Field[FieldIndex].name
    //return dbQuery->FieldByName(Field)->Value;
   /* if (!Field->required) {
        //bool aaa =  dbQuery->Fields->FindField(Field->name_src);
        //bool aaa1 =  dbQuery->Fields->FindField("adfa");
        //bool aaa2 =  dbQuery->Fields->FindField(Field->name_src);
        if (!dbQuery->Fields->FindField(Field->name_src))
            return "";
    }   */
    return dbQuery->FieldByName(Field->name_src)->Value;

}


//---------------------------------------------------------------------------
// ������������� �������� ��������� ����
void TStorageOraSql::setFieldValue(Variant Value)
{
    if (Fields[FieldIndex]->active && Fields[FieldIndex]->enable)
    {
        String name = Fields[FieldIndex]->name;
        dbQuery->FieldByName(name)->Value = Value;   // 2016-07-28
    }
}

//---------------------------------------------------------------------------
// ��������� ����� ������ � �������
void TStorageOraSql::append()
{
    dbQuery->Append();
    TStorage::append();
}

//---------------------------------------------------------------------------
// ��������� ���������
void TStorageOraProc::commit()
{
    if ( Modified )
    {
        try
        {
            dbSession->Commit();
        }
        catch (Exception &e)
        {
            throw Exception(e.Message);
        }
    }

    if(ReadOnly == false && Tables[TableIndex].FinalProcName != "" )  // ��������� �������������� ��������� ���� ��� ������
    {
        executeProc(Tables[TableIndex].FinalProcName);
    }
}

//---------------------------------------------------------------------------
// ��������� ���������
void TStorageOraSql::commit()
{
    if (ReadOnly)
    {
        throw Exception("Can't commit the storage because it is read-only.");
    }

    if ( dbQuery->Modified )
    {
        try
        {
            dbQuery->Post();
            dbSession->Commit();
            Modified = true;
            RecordCount = dbQuery->RecordCount;
        }
        catch (Exception &e)
        {
            //dbSession->Rollback();
            throw Exception(e.Message);
        }
    }

    if(ReadOnly == false && Tables[TableIndex].FinalProcName != "" )  // ��������� �������������� ��������� ���� ��� ������
    {
        executeProc(Tables[TableIndex].FinalProcName);
    }

    FieldIndex = 0;
}

//---------------------------------------------------------------------------
//
void TStorageOraSql::post()
{
    //dbQuery->ExecSQL();
    FieldIndex = 0;
}

//---------------------------------------------------------------------------
// ���������� true ���� ��������� ����� �������
bool TStorageOraSql::eor()
{
    if (dbQuery != NULL) {
        int R = dbQuery->RecordCount;
        int aa = dbQuery->RecNo;
        return dbQuery->Eof;
    } else {
        return true;
    }
}

//---------------------------------------------------------------------------
// ��������� � ��������� ������ �������
void TStorageOraSql::nextRecord()
{
    //int k = pTable->RecNo;
    dbQuery->Next();
    TStorage::nextRecord();
}

//---------------------------------------------------------------------------
// ���������� ��������������� ��������� ���� - ��� ���� � �������-���������
/*AnsiString TStorageOraSql::GetSrcField()
{
    return Fields[FieldIndex]->name_src;
}*/

//---------------------------------------------------------------------------
//
/*bool TStorageOraSql::IsActiveField()
{
    return Fields[FieldIndex]->active && Fields[FieldIndex]->enable;
} */

//---------------------------------------------------------------------------
//
TOraField* TStorageOraSql::addField()
{
    TOraField* Field = new TOraField();
    Fields.push_back(Field);
    FieldCount++;
    return Field;

}

//---------------------------------------------------------------------------
//
void TStorageOraSql::addTable(const TOraSqlTable &Table)
{
    Tables.push_back(Table);
    TableCount++;
}

//---------------------------------------------------------------------------
// ���������� ������������ ��������� ���������/��������� ������
AnsiString TStorageOraSql::getTable()
{
    if (!eot()) {
        if (Tables[TableIndex].Sql != "") {
            return Tables[TableIndex].Sql;
        } else {
            return Tables[TableIndex].Table;
        }
    } else {
        throw Exception("End of table is reached.");
    }
}
