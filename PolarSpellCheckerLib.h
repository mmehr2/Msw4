#ifndef POLAR_SPELL_CHECKER_LIB_INCLUDED
#define POLAR_SPELL_CHECKER_LIB_INCLUDED

#ifdef POLAREXPORT
	#undef POLAREXPORT
#endif


//constants and enumerations
typedef enum
{
	sceFileNotFound							=	301,
	sceNotValidMainDictionary				=	302,
	sceDictonaryNotOpened					=	303,
	sceIndexOutOfBounds						=	304,
	sceUnknownError							=	305,
	sceNotValidCustomDictionary				=	306,
	sceNoOpenedDictionaries					=	307,
	sceNotLinkedToTextControl				=	308,
	sceNotValidTextControl					=	309,
	sceEmptyString							=	310,
	sceNotValidAutoCorrectFile				=	311,
	sceAutocorrectReplacementNotAssigned	=	312,
	sceNotException							=	313,
	sceCannotGenerateStatistics				=	314,
	sceNotValidFilename						=	315,
	sceFileAlreadyExists					=	316,
	sceCannotCreateDictionary				=	317,
	sceNoOpenedCustomDictionaries			=	318,
	sceUnknownFileVersion					=	319,
	sceWordListFileNotFound					=	320,
	sceSptFileNotFound						=	321,
	sceLexFileNotFound						=	322,
	sceWordListFileInvalid					=	323,
	sceSptListFileInvalid					=	324,
	sceLexFileInvalid						=	325,
	sceCodePageNotSupported					=	326,
	sceDictionariesNotCompatibile			=	327,
	sceMemoryAllocError						=	328,
	sceInvalidObjectHandle					=	329,
	sceInvalidParameters					=	330,
	sceFileOpenError						=	331,
	sceFileCreateError						=	332
} sceErrorConstants;

typedef enum
{
	sceWordAdded				=	5001,
	sceCustomDictionaryCleared	=	5002,
	sceWordAddedToIgnore		=	5003,
	sceIgnoreAllCleared			=	5004,
	sceChangeAllCleared			=	5005,
	sceWordAddedToChangeAll		=	5006,
	sceWordEncountered			=	5009,
	sceWordCorrected			=	5008,
	sceBadWord					=	5007,
	sceNonModalDialogControlWasClicked = 5010,
	sceCustomSpellcheckingDialog=   5011,
	sceBadWordTyped				=	9001,
	sceWordCorrectedManually	=	9002,
	sceWordAutoCorrected		=	9003,

	sceOptionsDialogClosed		=	14005,
	sceAutoCorrectDialogClosed	=	14006,
	
	sceOnError					=	15001
} sceEventType;

typedef enum 
{
	ctUpperAlpha	= 0,
	ctLowerAlpha	= 1,
	ctNumber		= 2,
	ctPunctation	= 3,
    ctPunctationDelimiter = 4,
    ctPunctationConnector = 5,
	ctWhitespace	= 6,
	ctUndefined		= 7,
	ctAmbiguous		= 8
} ctCharType;

typedef enum
{
	scrEntireDocument	= 0,
	scrSelectionOnly	= 1,
	scrFromCursor		= 2
} scrCheckingRange;

typedef enum
{
	scrCheckingComplete		= 0,
	scrCancelButtonClicked	= 1,
	scrCanceledThroughEvent	= 2
} srSpellResult;

typedef enum
{
	scoSpellChecker		= 0,
	scoTextStatistics	= 1
} scoObjectType;

typedef enum sdaSpellDialogAction
    {	sdaShowDialog	= 0,
	sdaAddWord	= 1,
	sdaAutoCorrect	= 2,
	sdaCancel	= 3,
	sdaChange	= 4,
	sdaChangeAll	= 5,
	sdaIgnore	= 6,
	sdaIgnoreAll	= 7,
	sdaUndoLast	= 8,

    }	sdaSpellDialogAction;

#define POLAREXPORT extern "C" __declspec(dllexport)

//type definitions
typedef unsigned long HPOLAROBJECT;
typedef unsigned long HSPELL;
typedef unsigned long HTEXTSTATISTICS;
typedef void (CALLBACK *PSCALLBACKFUNC)(sceEventType, void*);


///////////////////////////////////////////////////////////////////////////////////////
//SpellChecker functions //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
POLAREXPORT long CALLBACK PSSetZoom(HSPELL hSpell, int iZoom);
POLAREXPORT long CALLBACK PSGetZoom(HSPELL hSpell, int* Result);

POLAREXPORT long CALLBACK PSGetModalSpellChecking(HSPELL hSpell, BOOL* Result);
POLAREXPORT long CALLBACK PSSetModalSpellChecking(HSPELL hSpell, BOOL bValue);
POLAREXPORT long CALLBACK PSGetSpellTopMostWindow(HSPELL hSpell, BOOL* Result);
POLAREXPORT long CALLBACK PSSetSpellTopMostWindow(HSPELL hSpell, BOOL bValue);
POLAREXPORT long CALLBACK PSSetSuggestionsOftenCapitalized(HSPELL hSpell, const char*  pszDictName, BOOL bWordsOftenCapitalized);
POLAREXPORT long CALLBACK PSSetSuggestionsRules(HSPELL hSpell, const char*  pszDictName, const char* pszRulesFileName);
POLAREXPORT long CALLBACK PSSetSuggestionsSearchDepth(HSPELL hSpell, const char*  pszDictName, int lDepth);
POLAREXPORT long CALLBACK PSWriteLabelsTemplate(HSPELL hSpell, const char*  pszLabelsFileName);
POLAREXPORT long CALLBACK PSReadLabelsFromFile(HSPELL hSpell, const char*  pszLabelsFileName,const char*  pszErrorsFileName );

POLAREXPORT long CALLBACK PSCreateSpellChecker(HSPELL* Result);
POLAREXPORT long CALLBACK PSDestroySpellChecker(HSPELL hSpell);
POLAREXPORT long CALLBACK PSOpenDictionary (HSPELL hSpell, const char*  pszDictFileName);
POLAREXPORT long CALLBACK PSCloseDictionary (HSPELL hSpell, const char*  pszDictFileName);
POLAREXPORT long CALLBACK PSGetDictionaryPath (HSPELL hSpell, int nIndex , char* Result, long lBufLen);
POLAREXPORT long CALLBACK PSOpenCustomDictionary (HSPELL hSpell, const char*  pszCustomDictFileName);
POLAREXPORT long CALLBACK PSCloseCustomDictionary (HSPELL hSpell, const char*  pszCustomDictFileName);
POLAREXPORT long CALLBACK PSGetCustomDictionaryPath(HSPELL hSpell, int nIndex, char* Result, long lBufLen);
POLAREXPORT long CALLBACK PSCreateCustomDictionary(HSPELL hSpell, const char*  pszCustomDictFileName);
POLAREXPORT long CALLBACK PSCheckText(HSPELL hSpell, const char*  pszText, srSpellResult* pExitStatus, char* Result, long lBufLen);
POLAREXPORT long CALLBACK PSAddWord(HSPELL hSpell, const char*  pszNewWord);
POLAREXPORT long CALLBACK PSAddToChangeAll(HSPELL hSpell, const char*  pszFind, const char*  pszReplace);
POLAREXPORT long CALLBACK PSAddToIgnoreAll(HSPELL hSpell, const char*  pszWord);
POLAREXPORT long CALLBACK PSClearIgnoreAll(HSPELL hSpell);
POLAREXPORT long CALLBACK PSClearChangeAll(HSPELL hSpell);
POLAREXPORT long CALLBACK PSDoesWordExist(HSPELL hSpell, const char* pszWord, BOOL* Result);
POLAREXPORT long CALLBACK PSEmptyCustomDictionary (HSPELL hSpell, const char*  pszDictFileName);
POLAREXPORT long CALLBACK PSCharType(HSPELL hSpell, int ch, ctCharType* Result);
POLAREXPORT long CALLBACK PSGetLanguageName(HSPELL hSpell, const char* pszDictFileName, char* Result, long lBufLen);
POLAREXPORT long CALLBACK PSCheckTextControl (HSPELL hSpell, scrCheckingRange CheckingRange, srSpellResult* Result) ;
POLAREXPORT long CALLBACK PSOptionsDlg(HSPELL hSpell, BOOL* Result);
POLAREXPORT long CALLBACK PSGetReplacement(HSPELL hSpell, const char* pszWord, char* Result, long lBufLen);
POLAREXPORT long CALLBACK PSGetSuggestionCount(HSPELL hSpell, const char* pszWord, int* Result);
POLAREXPORT long CALLBACK PSGetSuggestion(HSPELL hSpell, const char* pszWord, int nSuggestionIndex, char* Result, long lBufLen);
POLAREXPORT long CALLBACK PSGenerateStatisticsFromString (HSPELL hSpell, const char* pszText, HTEXTSTATISTICS* Result);
POLAREXPORT long CALLBACK PSGenerateStatisticsFromTextControl (HSPELL hSpell, HWND hHandle, HTEXTSTATISTICS* Result);
POLAREXPORT long CALLBACK PSInitializeAutoCorrection(HSPELL hSpell, const char* pszAutoCorrectFileName);
POLAREXPORT long CALLBACK PSSaveInitializationFile(HSPELL hSpell, const char* pszAutoCorrectFileName);
POLAREXPORT long CALLBACK PSCorrectText(HSPELL hSpell, const char* pszText, char* pCorrectedText, long lBufLen, BOOL* Result) ;
POLAREXPORT long CALLBACK PSAutoCorrectOptionsDlg(HSPELL hSpell, BOOL* Result) ;
POLAREXPORT long CALLBACK PSAddAutoCorrectReplacement (HSPELL hSpell, const char* pszWordToBeReplaced, const char* pszReplacement);
POLAREXPORT long CALLBACK PSRemoveAutoCorrectReplacement (HSPELL hSpell, const char* pszWordToBeReplaced);
POLAREXPORT long CALLBACK PSGetAutoCorrectReplacement (HSPELL hSpell, int nIndex, char* pWordToBeReplaced, long lWordBufLen, char* pReplacement, long lReplacementBufLen );
POLAREXPORT long CALLBACK PSAddFirstLetterException (HSPELL hSpell, const char* pszException);
POLAREXPORT long CALLBACK PSRemoveFirstLetterException (HSPELL hSpell, const char* pszException);
POLAREXPORT long CALLBACK PSGetFirstLetterException (HSPELL hSpell, int nIndex, char* Result, long lBufLen);
POLAREXPORT long CALLBACK PSAddTwoInitCapsException (HSPELL hSpell, const char* pszException);
POLAREXPORT long CALLBACK PSRemoveTwoInitCapsException (HSPELL hSpell, const char* pszException);
POLAREXPORT long CALLBACK PSGetTwoInitCapsException (HSPELL hSpell, int nIndex, char* Result, long lBufLen);
POLAREXPORT long CALLBACK PSAddOtherException (HSPELL hSpell, const char* pszException);
POLAREXPORT long CALLBACK PSRemoveOtherException (HSPELL hSpell, const char*  pszException);
POLAREXPORT long CALLBACK PSGetOtherException (HSPELL hSpell, int nIndex,  char* Result, long lBufLen);
POLAREXPORT long CALLBACK PSGetDictionaryCount(HSPELL hSpell, int* Result);
POLAREXPORT long CALLBACK PSGetCustomDictionaryCount(HSPELL hSpell, int* Result);
POLAREXPORT long CALLBACK PSSetActiveCustomDictionaryPath(HSPELL hSpell, const char* pszPath);
POLAREXPORT long CALLBACK PSGetActiveCustomDictionaryPath(HSPELL hSpell, char* Result, long lBufLen);
POLAREXPORT long CALLBACK PSSetAlwaysSuggest(HSPELL hSpell, BOOL bAlwaysSuggest);
POLAREXPORT long CALLBACK PSGetAlwaysSuggest(HSPELL hSpell, BOOL* Result);
POLAREXPORT long CALLBACK PSSetIgnoreWordsInUppercase(HSPELL hSpell, BOOL bIgnoreWordsInUppercase);
POLAREXPORT long CALLBACK PSGetIgnoreWordsInUppercase(HSPELL hSpell, BOOL* Result);
POLAREXPORT long CALLBACK PSSetIgnoreWordsWithNumbers(HSPELL hSpell, BOOL bIgnoreWordsWithNumbers);
POLAREXPORT long CALLBACK PSGetIgnoreWordsWithNumbers(HSPELL hSpell, BOOL* Result);
POLAREXPORT long CALLBACK PSSetLinkedTextControlHandle(HSPELL hSpell, HWND hHandle);
POLAREXPORT long CALLBACK PSGetLinkedTextControlHandle(HSPELL hSpell, HWND* Result);
POLAREXPORT long CALLBACK PSSetCheckSpellingAsYouType(HSPELL hSpell, BOOL bCheckSpellingAsYouType);
POLAREXPORT long CALLBACK PSGetCheckSpellingAsYouType(HSPELL hSpell, BOOL* Result);
POLAREXPORT long CALLBACK PSSetChangePopup(HSPELL hSpell, BOOL bChangePopup);
POLAREXPORT long CALLBACK PSGetChangePopup(HSPELL hSpell, BOOL* Result);
POLAREXPORT long CALLBACK PSSetIgnoreInternetAndFileAddresses(HSPELL hSpell, BOOL bIgnoreInternetAndFileAddresses);
POLAREXPORT long CALLBACK PSGetIgnoreInternetAndFileAddresses(HSPELL hSpell, BOOL* Result);
POLAREXPORT long CALLBACK PSSetSuggestFromMainDictionariesOnly(HSPELL hSpell, BOOL bSuggestFromMainDictsOnly);
POLAREXPORT long CALLBACK PSGetSuggestFromMainDictionariesOnly(HSPELL hSpell, BOOL* Result);

POLAREXPORT long CALLBACK PSSetUnderlineCurrentlyTypedWord(HSPELL hSpell, BOOL bValue);
POLAREXPORT long CALLBACK PSGetUnderlineCurrentlyTypedWord(HSPELL hSpell, BOOL* Result);
POLAREXPORT long CALLBACK PSSetCaseSensitiveCustomDictionaries(HSPELL hSpell, BOOL bValue);
POLAREXPORT long CALLBACK PSGetCaseSensitiveCustomDictionaries(HSPELL hSpell, BOOL* Result);

POLAREXPORT long CALLBACK PSSetCorrectTwoInitCaps(HSPELL hSpell, BOOL bCorrectTwoInitCaps);
POLAREXPORT long CALLBACK PSGetCorrectTwoInitCaps(HSPELL hSpell, BOOL* Result);
POLAREXPORT long CALLBACK PSSetCorrectCapsLock(HSPELL hSpell, BOOL bCorrectCapsLock);
POLAREXPORT long CALLBACK PSGetCorrectCapsLock(HSPELL hSpell, BOOL* Result);
POLAREXPORT long CALLBACK PSSetCapitalizeSentence(HSPELL hSpell, BOOL bCapitalizeSentence);
POLAREXPORT long CALLBACK PSGetCapitalizeSentence(HSPELL hSpell, BOOL* Result);
POLAREXPORT long CALLBACK PSSetAutoCorrectTextAsYouType(HSPELL hSpell, BOOL bAutoCorrectTextAsYouType);
POLAREXPORT long CALLBACK PSGetAutoCorrectTextAsYouType(HSPELL hSpell, BOOL* Result);
POLAREXPORT long CALLBACK PSSetAutoAddWordsToExceptionList (HSPELL hSpell, BOOL bAutoAddWordsToExceptionList);
POLAREXPORT long CALLBACK PSGetAutoAddWordsToExceptionList (HSPELL hSpell, BOOL* Result);
POLAREXPORT long CALLBACK PSGetAutoCorrectReplacementCount(HSPELL hSpell, int* Result);
POLAREXPORT long CALLBACK PSGetFirstLetterExceptionCount(HSPELL hSpell, int* Result);
POLAREXPORT long CALLBACK PSGetOtherExceptionCount(HSPELL hSpell, int* Result);
POLAREXPORT long CALLBACK PSGetTwoInitCapsExceptionCount(HSPELL hSpell, int* Result);
POLAREXPORT long CALLBACK PSShowTextControlPopup(HSPELL hSpell);
POLAREXPORT long CALLBACK PSShowTextControlPopupAtPos(HSPELL hSpell, int x, int y);
POLAREXPORT long CALLBACK PSGetWordsCount(HSPELL hSpell, long* Result);


///////////////////////////////////////////////////////////////////////////////////////
//Dictionary generator
///////////////////////////////////////////////////////////////////////////////////////

POLAREXPORT long CALLBACK PSCreateLexFromSpt(HSPELL hSpell, const char*  pszDicitionaryName, const char*   pszDictionaryDescription ,								const char*  pszWordListFile, const char* pszSptFile, const char*  pszOutputLexFile, BOOL bCaseSensitive);
POLAREXPORT long CALLBACK PSCreateLexFromCodepage(HSPELL hSpell, const char*  pszDicitionaryName, const char*   pszDictionaryDescription,									 const char*  pszWordListFile, const char* pszCodePage, const char* pszOutputLexFile, BOOL bCaseSensitive);
POLAREXPORT long CALLBACK PSExtractWordsFromLex(HSPELL hSpell, const char*  pszDicitionaryName, const char*  pszOutputWordList);
POLAREXPORT long CALLBACK PSExtractSptFromLex(HSPELL hSpell, const char*  pszDicitionaryFile, const char*  pszOutputSptFile);
POLAREXPORT long CALLBACK PSExportSptToLex(HSPELL hSpell, const char*  pszDicitionaryFile, const char*  pszInputSptFile);
POLAREXPORT long CALLBACK PSExportWordsToLex(HSPELL hSpell, const char* pszDicitionaryFile, const char* pszInputWordList);


///////////////////////////////////////////////////////////////////////////////////////
//TextStatistics functions ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

POLAREXPORT long CALLBACK PSGetSpecificPhraseCount(HTEXTSTATISTICS hStatObject, const char* pszWord, int* Result);
POLAREXPORT long CALLBACK PSWritePhraseHistogramToFile(HTEXTSTATISTICS hStatObject, const char* pszFileName);
POLAREXPORT long CALLBACK PSCombineStatistics(HTEXTSTATISTICS hStatObject, HTEXTSTATISTICS hAnotherStatObject,								HTEXTSTATISTICS* Result);
POLAREXPORT long CALLBACK PSDestroyTextStatistics(HTEXTSTATISTICS hStatObject);
POLAREXPORT long CALLBACK PSGetWordCount(HTEXTSTATISTICS hStatObject, int* Result) ;
POLAREXPORT long CALLBACK PSGetWrongWordCount(HTEXTSTATISTICS hStatObject, int* Result);
POLAREXPORT long CALLBACK PSGetLineCount (HTEXTSTATISTICS hStatObject, int* Result);
POLAREXPORT long CALLBACK PSGetCharacterWithSpacesCount(HTEXTSTATISTICS hStatObject, int* Result);
POLAREXPORT long CALLBACK PSGetCharacterWithoutSpacesCount(HTEXTSTATISTICS hStatObject, int* Result);
POLAREXPORT long CALLBACK PSGetPhraseHistogram (HTEXTSTATISTICS hStatObject, char* Result, long lBufLen);
POLAREXPORT long CALLBACK PSGetPhraseHistogramLength(HTEXTSTATISTICS hStatObject, int* Result);
POLAREXPORT long CALLBACK PSCalculateDifference(HTEXTSTATISTICS hStatObject, HTEXTSTATISTICS hAnotherStatObject, HTEXTSTATISTICS* Result);


/////////////////////////////////////////////////////////////////////////////////
//Universal functions ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

POLAREXPORT long CALLBACK PSSetAllowErrorReporting(HPOLAROBJECT hObject, BOOL bEnable);
POLAREXPORT long CALLBACK PSGetAllowErrorReporting(HPOLAROBJECT hObject, BOOL* Result);

POLAREXPORT long CALLBACK PSSetEventCallbackFunc(HPOLAROBJECT hObject, PSCALLBACKFUNC pFunc);
POLAREXPORT long CALLBACK PSGetEventCallbackFunc (HPOLAROBJECT hObject, PSCALLBACKFUNC* Result);


/////////////////////////////////////////////////////////////////////////////////
//Event structures //////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////


struct CUSTOM_SPELLCHECKING_DIALOG_EVENT_INFO
{
	HSPELL hSpell;
	const char* pszBadWord;
	char* pszBadWordCorrection;
	int nBadWordCorrectionBufferLength;
	int nBadWordPosition;
	sdaSpellDialogAction *penumSpellDialogAction;
	BOOL bCanUndo;
};

struct BAD_WORD_EVENT_INFO
{
	HSPELL hSpell;
	const char* pszWord;
	BOOL* bSkip;
};

struct WORD_CORRECTED_EVENT_INFO
{
	HSPELL hSpell;
	const char* pszBadWord;	
	const char* pszCorrection;
	int nCorrectionIndex;
};

struct WORD_AUTOCORRECTED_EVENT_INFO
{
	HSPELL hSpell;
	HWND hHandle;
	const char* pszWord;
	const char* pszReplacement;
};

struct BAD_WORD_TYPED_EVENT_INFO
{
	HSPELL hSpell;
	HWND hHandle;
	const char* pszBadWord;
};

struct WORD_CORRECTED_MANUALLY_EVENT_INFO
{
	HSPELL hSpell;
	HWND hHandle;
	const char* pszBadWord;
	const char*  pszCorrection;
	int nCorrectionIndex;
};

struct WORD_ENCOUNTERED_EVENT_INFO
{
	HSPELL hSpell;
	BOOL* bCancel;
};

struct WORD_ADDED_EVENT_INFO
{
	HSPELL hSpell;
	const char*  pszWord;
	const char*  pszCustomDictionary;
	BOOL bAlreadyExists;
};

struct CUSTOM_DICTIONARY_CLEARED_EVENT_INFO
{
	HSPELL hSpell;
	const char*  pszCustomDictionary;
};

struct WORD_ADDED_TO_IGNORE_EVENT_INFO
{
	HSPELL hSpell;
	const char*  pszWord;
	BOOL bAlreadyExists;
};

struct WORD_ADDED_TO_CHANGE_ALL_EVENT_INFO
{
	HSPELL hSpell;
	const char*  pszWord;
	const char*  pszReplacement;
	BOOL bAlreadyExists;
};

struct IGNORE_ALL_CLEARED_EVENT_INFO
{
	HSPELL hSpell;
};

struct CHANGE_ALL_CLEARED_EVENT_INFO
{
	HSPELL hSpell;
};

struct NON_MODAL_DIALOG_CONTROL_WAS_CLICKED_EVENT_INFO
{	
	HSPELL hSpell;
};
struct OPTIONS_DIALOG_CLOSED_EVENT_INFO
{
	HSPELL hSpell;
	BOOL bCanceled;
	BOOL bAlwaysSuggest;
	BOOL bIgnoreWordsInUppercase;
	BOOL bIgnoreWordsWithNumbers;
	BOOL bIgnoreAdresses;
	BOOL bCheckSpellingAsYouType;
	BOOL bSuggestFromMainDictsOnly;

	const char*  pszActiveCustomDictionary;
	
	const char** ppOpenedCustomDictionaries;
	int nOpenedCustomDictionaryCount;

	const char** ppOpenedMainDictionaries;
	int nOpenedMainDictionaryCount;

	const char** ppCustomDictionariesList;
	int nCustomDictionaryListCount;

	const char** ppMainDictionariesList;
	int nMainDictionaryListCount;
};

struct AUTOCORRECT_DIALOG_CLOSED_EVENT_INFO
{
	HSPELL hSpell;
	BOOL bCanceled;
	BOOL bCorrectTwoInitCaps;
	BOOL bCorrectCapsLock;
	BOOL bCapitalizeSentence;
	BOOL bAutoCorrectTextAsYouType;
	BOOL bAutoAddWordsToExceptionList;

	const char** ppFirstLetterExceptions;
	int nFirstLetterExceptionCount;

	const char** ppInitialCapsExceptions;
	int nInitialCapsExceptionCount;

	const char** ppOtherExceptions;
	int nOtherExceptionCount;

	const char** ppWordsToBeReplaced;
	int nWordsToBeReplacedCount;

	const char** ppReplacements;
	int nReplacementCount;
};

struct ON_ERROR_EVENT_INFO
{
	HPOLAROBJECT hObj;
	scoObjectType nObjectType;
	sceErrorConstants nError;
	const char*  pszErrorDescription;
};

#endif // !defined(POLAR_SPELL_CHECKER_LIB_INCLUDED)