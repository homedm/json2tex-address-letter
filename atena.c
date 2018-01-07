/*年賀状などのあて名書き面を生成するtexファイルを作り、コンパイルまで行う*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "./parson/parson.h" //json読み込み用のライブラリ

#define TCHAR wchar_t
#define NAME_MAX 1012
#define PREFECTURES_MAX 1012
#define CITIES_MAX 1012
#define APARTMENT_MAX 1012

//parsonのラップ {{{
#define JSON2STRUCT_STR(_json, _struct, _key, _size)                       \
		do{                                                                          \
				strncpy(_struct._key, json_object_dotget_string(_json, #_key),_size); \
		} while (0);                                                                \

#define JSON2STRUCT_NUM(_json, _struct, _key)                  \
		do {                                                         \
				_struct._key = json_object_dotget_number(_json, #_key); \
		}while(0); 

#define JSON2STRUCT_BOOL(_json, _struct, _key)                 \
		do {                                                         \
				_struct._key = json_object_dotget_boolean(_json, #_key);    \
		}while(0); 
//}}}


// 構造体宣言 {{{
//struct _NAMEをNameとして宣言
typedef struct _NAME {
		char first[NAME_MAX + 1];
		char last[NAME_MAX + 1];
} Name;

typedef struct _Address{
		int postal_code; //郵便番号
		char prefectures[PREFECTURES_MAX + 1]; //都道府県
		char cities[CITIES_MAX + 1]; //市町村
		char apartment[APARTMENT_MAX + 1]; //アパート名、部屋番号
}Address;
//struct _PERSONINFOをPERSON_INFOとして宣言
//PERSON_INFOを宣言するときにstructをつけなくてもよくなる
typedef struct _PERSONINFO {
		Name name;
		Address address;
}PERSON_INFO; // }}}

/* json sample {{{
 *{
 *		"send":[
 *				{
 *						"name": {
 *								"first": "豊臣",
 *								"last": "秀吉"
 *						},
 *						"address": {
 *								"postal_code": 123456,
 *								"prefectures": "東京",
 *								"cities": "大手町 1-1-1",
 *								"apartment": "読売新聞社 123螳､"
 *						}
 *				},
 *				{
 *						"name": {
 *								"first": "徳川",
 *								"last": "家康"
 *						},
 *						"address":{
 *								"postal_code": 789123,
 *								"prefectures": "大阪県",
 *								"cities": "大阪市 2-3-4",
 *								"apartment": "姫路乗 456"
 *						}
 *				}
 *		],
 *		"self": {
 *				"name": {
 *						"first": "高橋",
 *						"last": "純一"
 *				},
 *				"address": {
 *						"postal_code": 456789,
 *						"prefectures": "千葉県",
 *						"cities": "千葉市 1-1-1",
 *						"apartment": ""
 *				}
 *		}
 * }
 * }}} */
int main(int argc, char *argv[]) {
		//引数がなかった場合エラーを返す {{{
		if (argc != 2) { 
				fprintf(stderr, "error: not indicate json filename\n");
				exit(1);
		} //}}}

//parseするjsonファイルを指定
//errorの場合NULLを返す
JSON_Value *root_value = json_parse_file(argv[1]);

//jsonファイルを開けなかった場合のerror処理 {{{
if ( root_value == NULL ){
		fprintf(stderr, "error: fail to open the file:%s\n", argv[1] );
		exit(1);
} // }}}

FILE* outFile = fopen("letter_address.tex", "w");
//error 処理 {{{
if (!outFile) { 
		perror("error: letter_address.tex");
		exit(1);
}
//}}}

JSON_Object *root = json_object(root_value);
//selfinfoに自分の住所を格納する {{{
PERSON_INFO selfinfo;
JSON_Object *self = json_object_get_object(root, "self");
JSON2STRUCT_STR(self, selfinfo, name.first, NAME_MAX);
JSON2STRUCT_STR(self, selfinfo, name.last, NAME_MAX);
JSON2STRUCT_NUM(self, selfinfo, address.postal_code);
JSON2STRUCT_STR(self, selfinfo, address.prefectures, PREFECTURES_MAX);
JSON2STRUCT_STR(self, selfinfo, address.cities, CITIES_MAX);
JSON2STRUCT_STR(self, selfinfo, address.apartment, APARTMENT_MAX);
//}}}

//自分の宛名をファイルに書きこむ {{{
fprintf(outFile, "\\documentclass[]{jletteraddress}\n");
fprintf(outFile, "\n\\sendername{%s %s}\n", selfinfo.name.last, selfinfo.name.first);
fprintf(outFile, "\\senderaddressa{%s %s}\n", selfinfo.address.prefectures, selfinfo.address.cities);
fprintf(outFile, "\\senderaddressb{%s}\n", selfinfo.address.apartment);
fprintf(outFile, "\\senderpostcode{%d}\n\n", selfinfo.address.postal_code);
// }}}

//送り先の宛先を構造体に保存し、ファイルに書き込む {{{
//personsにjson内の配列sendの要素を代入する
JSON_Array *persons = json_object_get_array(root, "send");

fprintf(outFile, "\\begin{document}\n");
for(int i = 0; i < json_array_get_count(persons); i++) {
		//for(int i = 0; i < num; i++ ){
		JSON_Object *person = json_array_get_object(persons, i);

		//ファイルに書き込む {{{
		fprintf(outFile, "\\addaddress\n");
		fprintf(outFile, "    {%s %s}{様}\n", json_object_dotget_string(person, "name.last"), json_object_dotget_string(person, "name.first"));
		fprintf(outFile, "    {%d}\n", (int) json_object_dotget_number(person,"address.postal_code"));
		fprintf(outFile, "    {%s %s}\n", json_object_dotget_string(person, "address.prefectures"), json_object_dotget_string(person, "address.cities"));
		fprintf(outFile, "    {%s}\n", json_object_dotget_string(person, "address.apartment"));
		fprintf(outFile, "\n");
		// }}}
}
// }}}
fprintf(outFile, "\\end{document}");

fclose(outFile);
json_value_free(root_value);

printf("finished tex file: letter_address.tex");
//}}}

// compile
system("platex letter_address.tex");
system("dvipdfmx letter_address.dvi");
printf("finished letter pdf");
return 0;
}
