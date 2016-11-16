#include <iostream>
#include <string>
#include <list>
#include <fstream>
#include <algorithm>

using namespace std;

const int NAZWA_MAX = 8;

struct UCB
{
	char nazwa[NAZWA_MAX+1];
	int pierwszy_blok;
	int rozmiar;
	UCB();
	UCB(string nazwa_, int pierwszy_blok_, int rozmiar_); 
};


UCB::UCB()
{
}


UCB::UCB(string nazwa_, int pierwszy_blok_, int rozmiar_)
{
	strncpy(nazwa, nazwa_.c_str(), NAZWA_MAX);
	pierwszy_blok = pierwszy_blok_;
	rozmiar = rozmiar_;
}


struct Specjalne
{
	int rozmiar_pliku_z_katalogiem;
	int pierwszy_blok_pliku_z_katalogiem;
};


const int LICZBA_BLOKOW_DANE = 128;
const int ROZMIAR_BLOKU = 32;
const int LICZBA_BLOKOW_FAT = LICZBA_BLOKOW_DANE*sizeof(int)/ROZMIAR_BLOKU;
const int LICZBA_BLOKOW_SPECJALNE = 1;
const int LICZBA_BLOKOW = LICZBA_BLOKOW_DANE+LICZBA_BLOKOW_FAT+LICZBA_BLOKOW_SPECJALNE;
const int PIERWSZY_BLOK_KATALOGU = 0;


class Dysk
{
	unsigned char m_dane[LICZBA_BLOKOW*ROZMIAR_BLOKU];
public:
	void wczytaj_dysk(string nazwa);
	void zapisz_dysk(string nazwa);
	void wczytaj_blok(int indeks, void* bufor);
	void zapisz_blok(int indeks, void* bufor);
	unsigned char* get_dysk();
};

unsigned char* Dysk::get_dysk()
{
	return m_dane;
}

void Dysk::wczytaj_dysk(string nazwa)
{
	FILE* plik = fopen(nazwa.c_str(), "rb");
	if(plik)
	{
		fread(m_dane, 1, LICZBA_BLOKOW*ROZMIAR_BLOKU, plik);
		fclose(plik);
	}
	else
		cout << "Nie mozna otworzyc do odczytu pliku z dyskiem" << endl;
}

void Dysk::zapisz_dysk(string nazwa)
{
	FILE* plik = fopen(nazwa.c_str(), "wb");
	if(plik)
	{
		fwrite(m_dane, 1, LICZBA_BLOKOW*ROZMIAR_BLOKU, plik);
		fclose(plik);
	}
	else
		cout << "Nie mozna otworzyc do zapisu pliku z dyskiem" << endl;
}

void Dysk::wczytaj_blok(int indeks, void* bufor)
{
	memcpy(bufor, &m_dane[indeks*ROZMIAR_BLOKU], ROZMIAR_BLOKU);
}

void Dysk::zapisz_blok(int indeks, void* bufor)
{
	memcpy(&m_dane[indeks*ROZMIAR_BLOKU], bufor, ROZMIAR_BLOKU);
}



enum Tryb
{
	Tryb_ODCZYT,
	Tryb_ZAPIS
};

class SystemPlikow;
class UchwytPliku 
{
	int m_blok;
	int m_pozycja;
	int m_pozycja_blok;
	UCB *m_ucb;
	unsigned char m_cache[ROZMIAR_BLOKU];
	SystemPlikow *m_system_plikow;
	Tryb m_tryb;
	friend class SystemPlikow;
public:
	bool eof();
	UchwytPliku(SystemPlikow *system_plikow, UCB* ucb, Tryb tryb);
};


bool UchwytPliku::eof()		// spr czy wsk znajduje sie na koncu pliku
{
	return(m_tryb == Tryb_ODCZYT && m_pozycja == m_ucb->rozmiar);
}


UchwytPliku::UchwytPliku(SystemPlikow* system_plikow, UCB* ucb, Tryb tryb)
{
	m_system_plikow = system_plikow;
	m_ucb = ucb;
	m_tryb = tryb;
	m_blok = ucb->pierwszy_blok;
	m_pozycja = 0;
	m_pozycja_blok = 0;
};

class SystemPlikow
{
	int m_FAT[LICZBA_BLOKOW_DANE];
	list<UCB> m_katalog;
public:
	Dysk* m_dysk;
	SystemPlikow(Dysk* dysk);
	~SystemPlikow();
	int znajdz_wolny_blok();
	int czytaj_plik(UchwytPliku* uchwyt, void* bufor, int bajtow);
	int zapisz_plik(UchwytPliku* uchwyt, const void* bufor, int bajtow);
	UchwytPliku *otworz_do_odczytu(string nazwa);
	UchwytPliku *otworz_do_zapisu(string nazwa);
	void zamknij_plik(UchwytPliku* uchwyt);
	void usun_plik(string nazwa);
	void zapisz_katalog();
	UCB* znajdz_plik(string nazwa);
	static void formatuj(Dysk *dysk);
	void dir();
	void wyswietl_FAT();
	int get_katalog();
};

int SystemPlikow::get_katalog()
{
	return sizeof(m_katalog);
}

void SystemPlikow::zapisz_katalog()
{
	UchwytPliku *plik = otworz_do_zapisu("");
	int pierwszy_blok_pliku_z_katalogiem = plik->m_blok;
	int liczba_plikow = m_katalog.size()-1;						//jeden plik w liscie z UCB to plik z katalogiem, a tego nie liczymy
	zapisz_plik(plik, &liczba_plikow, sizeof(int));
	for(list<UCB>::iterator i = m_katalog.begin(), e = m_katalog.end(); i != e; ++i)
	{
		UCB *ucb = &(*i);
		if(ucb->nazwa[0] != 0)
			zapisz_plik(plik, ucb, sizeof(UCB));
	}
	int rozmiar_pliku_z_katalogiem = plik->m_pozycja;
	zamknij_plik(plik);

	char blok_specjalne[ROZMIAR_BLOKU];
	Specjalne *specjalne = (Specjalne*) blok_specjalne;
	specjalne->rozmiar_pliku_z_katalogiem = rozmiar_pliku_z_katalogiem;
	specjalne->pierwszy_blok_pliku_z_katalogiem = pierwszy_blok_pliku_z_katalogiem;
	m_dysk->zapisz_blok(LICZBA_BLOKOW - LICZBA_BLOKOW_FAT - 1, blok_specjalne);
}


void SystemPlikow::wyswietl_FAT()
{
	for (int i=0; i<LICZBA_BLOKOW_DANE; i++)
	{
		cout << m_FAT[i] << " ";
	}
	cout << endl;
}


void SystemPlikow::dir()
{
	cout << "LISTA PLIKOW" << "============" << endl << endl;
	for(list<UCB>::iterator i = m_katalog.begin(), e = m_katalog.end(); i != e; ++i)
	{
		UCB *ucb = &(*i);
		cout << ucb->nazwa << '\t';
		cout << ucb->pierwszy_blok << '\t';
		cout << ucb->rozmiar << endl;
	}
}


UCB* SystemPlikow::znajdz_plik(string nazwa)
{
	for(list<UCB>::iterator i = m_katalog.begin(), e = m_katalog.end(); i != e; ++i)
	{
		UCB* ucb = &(*i);
		if(!strcmp(ucb->nazwa, nazwa.c_str()))
			return(ucb);
	}
	return(NULL);
}


void SystemPlikow::formatuj(Dysk *dysk)
{
	int fat[LICZBA_BLOKOW_DANE];
	for(int i = 0; i < LICZBA_BLOKOW_DANE; ++i)
		fat[i] = -1;
	fat[0] = 0; // plik z katalogiem
	for(int i = 0; i < LICZBA_BLOKOW_FAT; ++i)
		dysk->zapisz_blok(LICZBA_BLOKOW - LICZBA_BLOKOW_FAT + i, &fat[i*ROZMIAR_BLOKU/sizeof(int)]);

	unsigned char blok_specjalne[ROZMIAR_BLOKU];
	Specjalne *specjalne = (Specjalne*) blok_specjalne;
	specjalne->pierwszy_blok_pliku_z_katalogiem = 0;
	specjalne->rozmiar_pliku_z_katalogiem = sizeof(int);
	dysk->zapisz_blok(LICZBA_BLOKOW - LICZBA_BLOKOW_FAT - 1, blok_specjalne);

	unsigned char blok_plik_z_katalogiem[ROZMIAR_BLOKU];		//pamiec na dane, ktore beda zapisane w bloku na dysku
	int *rozmiar = (int*) blok_plik_z_katalogiem;				//wpisanie zera do tych danych
	*rozmiar = 0;
	dysk->zapisz_blok(0, blok_plik_z_katalogiem);				//zapisanie bloku
}


SystemPlikow::SystemPlikow(Dysk* dysk)
{
	m_dysk = dysk;
	for(int i=0; i<LICZBA_BLOKOW_FAT; i++)
		dysk->wczytaj_blok(LICZBA_BLOKOW-LICZBA_BLOKOW_FAT+i, &m_FAT[i*ROZMIAR_BLOKU/sizeof(int)]);

	int pierwszy_blok_specjalne = LICZBA_BLOKOW - LICZBA_BLOKOW_FAT - LICZBA_BLOKOW_SPECJALNE;  //odczytane bloku sepcjalnego z informacja o rozmiarze pliku z katalogiem oraz tego pliku do listy struktur UCB
	unsigned char specjalne_blok[ROZMIAR_BLOKU];
	m_dysk->wczytaj_blok(pierwszy_blok_specjalne, &specjalne_blok[0]);
	Specjalne *specjalne = (Specjalne*) specjalne_blok;
	m_katalog.push_back(UCB("", specjalne->pierwszy_blok_pliku_z_katalogiem, specjalne->rozmiar_pliku_z_katalogiem));
	
	UchwytPliku *plik = otworz_do_odczytu("");				//otwrarcie plik z katalogiem i wczytanie jego zawartosc
	int liczba_plikow;
	czytaj_plik(plik, &liczba_plikow, sizeof(int));
	for(int i = 0; i < liczba_plikow; ++i)
	{
		UCB ucb;
		czytaj_plik(plik, &ucb, sizeof(UCB));
		m_katalog.push_back(ucb);
	}
	zamknij_plik(plik);
}

SystemPlikow::~SystemPlikow()
{
	//UCB *ucb_katalog = znajdz_plik("");
	//int
	
	/*UchwytPliku *plik = otworz_do_zapisu("");
	int pierwszy_blok_pliku_z_katalogiem = plik->m_blok;
	int liczba_plikow = m_katalog.size()-1;						//jeden plik w liscie z UCB to plik z katalogiem, a tego nie liczymy
	zapisz_plik(plik, &liczba_plikow, sizeof(int));
	for(list<UCB>::iterator i = m_katalog.begin(), e = m_katalog.end(); i != e; ++i)
	{
		UCB *ucb = &(*i);									// "czyszczenie" katalogu
		zapisz_plik(plik, ucb, sizeof(UCB));
	}
	int rozmiar_pliku_z_katalogiem = plik->m_pozycja;
	zamknij_plik(plik);

	char blok_specjalne[ROZMIAR_BLOKU];
	Specjalne *specjalne = (Specjalne*) blok_specjalne;
	specjalne->rozmiar_pliku_z_katalogiem = rozmiar_pliku_z_katalogiem;
	specjalne->pierwszy_blok_pliku_z_katalogiem = pierwszy_blok_pliku_z_katalogiem;
	m_dysk->zapisz_blok(LICZBA_BLOKOW - LICZBA_BLOKOW_FAT - 1, blok_specjalne);
	*/
	for(int i=0; i<LICZBA_BLOKOW_FAT; i++)
		m_dysk->zapisz_blok(LICZBA_BLOKOW-LICZBA_BLOKOW_FAT+i, &m_FAT[i*ROZMIAR_BLOKU/sizeof(int)]);		// "czyszczenie" dysku
}

int SystemPlikow::znajdz_wolny_blok()
{
	for (int i=0; i<LICZBA_BLOKOW_DANE; i++)
	{
		if(m_FAT[i]==-1)
			return i;
	}
	return -1;
}

int SystemPlikow::czytaj_plik(UchwytPliku* uchwyt, void* bufor, int bajtow)
{
	int przeczytane = 0;
	unsigned char *b = (unsigned char*)bufor;
	while(true)
	{
		int pozostalo = uchwyt->m_ucb->rozmiar-uchwyt->m_pozycja;		//pozycja w pliku (liczba odczytanych lub zapisanych bajtow), liczba bajtow do konca pliku
		int pozostalo_blok = ROZMIAR_BLOKU-uchwyt->m_pozycja_blok;		//pozycja w aktualnym bloku na dysku, liczba bajtow do konca bloku
		int n = min(min(bajtow, pozostalo_blok), pozostalo);
		memcpy(b, &uchwyt->m_cache[uchwyt->m_pozycja_blok], n);         // umieszczanie z cache do bufora czytanego pliku
		b+=n;
		uchwyt->m_pozycja+=n;
		uchwyt->m_pozycja_blok+=n;
		przeczytane+=n;
		bajtow-=n;
		if(bajtow>0 && pozostalo>0)
		{
			if(uchwyt->m_pozycja_blok == ROZMIAR_BLOKU)
			{
				int nastepny_blok = m_FAT[uchwyt->m_blok];
				if(nastepny_blok!=uchwyt->m_blok)
				{
					uchwyt->m_blok = nastepny_blok;
					uchwyt->m_pozycja_blok = 0;
					m_dysk->wczytaj_blok(nastepny_blok, uchwyt->m_cache);
				}
				else
					break;
			}
		}
		else
			break;
	}
	return przeczytane;
}

int SystemPlikow::zapisz_plik(UchwytPliku* uchwyt, const void* bufor, int bajtow)
{
	int zapisane = 0;
	const unsigned char *b = (const unsigned char*)bufor;
	while(true)
	{
		int pozostalo_blok = ROZMIAR_BLOKU-uchwyt->m_pozycja_blok;
		int n = min(bajtow, pozostalo_blok);						// min wartosc z ilosci bajtow i pozycji w bloku
		memcpy(&uchwyt->m_cache[uchwyt->m_pozycja_blok], b, n);    // zapisanie bufora cache
		uchwyt->m_pozycja_blok+=n;
		uchwyt->m_pozycja+=n;
		bajtow-=n;
		zapisane+=n;
		b += n;
		if(bajtow>0)
		{
			if (uchwyt->m_pozycja_blok==ROZMIAR_BLOKU)
			{
				m_dysk->zapisz_blok(uchwyt->m_blok, uchwyt->m_cache);     // zapisanie na dysk pliku z pamieci cache
				int nowy_blok = znajdz_wolny_blok();
				m_FAT[uchwyt->m_blok] = nowy_blok;
				m_FAT[nowy_blok] = nowy_blok;
				uchwyt->m_blok = nowy_blok;
				uchwyt->m_pozycja_blok = 0;
			}
		}
		else
			break;
	}
	return zapisane;
}

UchwytPliku* SystemPlikow::otworz_do_odczytu(string nazwa)
{
	for(list<UCB>::iterator i=m_katalog.begin(), e=m_katalog.end(); i!=e; i++)
	{
		UCB *ucb = &(*i);
		if(!strcmp(ucb->nazwa, nazwa.c_str()))
		{
			UchwytPliku *uchwyt = new UchwytPliku(this, ucb, Tryb_ODCZYT);
			m_dysk->wczytaj_blok(uchwyt->m_blok, uchwyt->m_cache);
			return uchwyt;
		}
		
	}
	return NULL;
}

UchwytPliku* SystemPlikow::otworz_do_zapisu(string nazwa)
{
	if (nazwa.length()<=NAZWA_MAX)
	{
		usun_plik(nazwa);
		m_katalog.push_back(UCB());
		UCB *ucb = &m_katalog.back();
		strcpy(ucb->nazwa, nazwa.c_str());
		ucb->pierwszy_blok = znajdz_wolny_blok();
		ucb->rozmiar = 0;
		m_FAT[ucb->pierwszy_blok] = ucb->pierwszy_blok;
		UchwytPliku *uchwyt = new UchwytPliku(this, ucb, Tryb_ZAPIS);
		return uchwyt;
	}
	return NULL;
}

void SystemPlikow::zamknij_plik(UchwytPliku* uchwyt)
{
	if(uchwyt->m_tryb == Tryb_ZAPIS)
	{
		m_dysk->zapisz_blok(uchwyt->m_blok, uchwyt->m_cache);
		uchwyt->m_ucb->rozmiar = uchwyt->m_pozycja;
	}
	if(uchwyt->m_ucb != znajdz_plik(""))
		zapisz_katalog();
	delete uchwyt;
}

void SystemPlikow::usun_plik(string nazwa)
{
	for (list<UCB>::iterator i=m_katalog.begin(), e=m_katalog.end(); i!=e; i++)
	{
		UCB *ucb = &(*i);
		if(!strcmp(ucb->nazwa, nazwa.c_str()))
		{
			int blok = ucb->pierwszy_blok;
			while(true)
			{
				int nastepny_blok = m_FAT[blok];
				m_FAT[blok] = -1;
				if(nastepny_blok == blok)
					break;
				blok = nastepny_blok;
			}
			m_katalog.erase(i);
			if(nazwa.size())
				zapisz_katalog();
			break;
		}
	}
}

bool czytaj_linie(SystemPlikow *system_plikow, UchwytPliku *plik, string &linia)
{
	bool wynik = false;
	linia = "";
	while(plik->eof()==false)
	{
		wynik = true;
		char c;
		system_plikow->czytaj_plik(plik, &c, 1);
		if (c=='\n')
			break;
		linia.push_back(c);

	}
	return wynik;
}


struct PAMIEC
{
	string element[100];
};


int main()
{
		//cout << sizeof(UCB) << endl;
	Dysk *dysk = new Dysk();
	//dysk->wczytaj_dysk("dysk.txt");
	SystemPlikow::formatuj(dysk);
	SystemPlikow *system_plikow = new SystemPlikow(dysk);
	//system_plikow->dir();


	/*UchwytPliku *plik2 = system_plikow->otworz_do_zapisu("Program");
	system_plikow->zapisz_plik(plik2, "$JOB,50\nvoid dodaj()\n{int a=2,b=3;\ncout<<a+b;}\n", 50);
	system_plikow->zamknij_plik(plik2);

	char bufor[100];
	system_plikow->otworz_do_odczytu("Program");
	system_plikow->czytaj_plik(plik2, bufor, 52);
	bufor[52] = '\0';
	cout << bufor << endl;
	system_plikow->dir();*/


	/*UchwytPliku *plik = system_plikow->otworz_do_zapisu("D");
	system_plikow->zapisz_plik(plik, "$JOB,50\nvoid dodaj",20);
	system_plikow->zapisz_plik(plik, "()\n{int a=2,b=3;" ,20);
	system_plikow->zapisz_plik(plik, "\ncout<<a+b;}\n$END",20);
	//system_plikow->zapisz_plik(plik, "abcdefghijklmnopqrst",20);
	system_plikow->zamknij_plik(plik);

	system_plikow->dir();

	UchwytPliku *plik1 = system_plikow->otworz_do_zapisu("M");
	system_plikow->zapisz_plik(plik1, "$JOB,50\nvoid mnoz",20);
	system_plikow->zapisz_plik(plik1, "()\n{int a=2,b=3;" ,20);
	system_plikow->zapisz_plik(plik1, "\ncout<<a*b;}\n$END",20);
	//system_plikow->zapisz_plik(plik, "abcdefghijklmnopqrst",20);
	system_plikow->zamknij_plik(plik1);
	system_plikow->dir();*/

	//delete system_plikow;
	//system_plikow = new SystemPlikow(dysk);

	/*UchwytPliku *plik = system_plikow->otworz_do_odczytu("xy");
	system_plikow->czytaj_plik(plik, bufor, 10);
	system_plikow->czytaj_plik(plik, &bufor[10], 70);
	system_plikow->zamknij_plik(plik);
	bufor[80] = '\0';
	cout << bufor << endl;
	system_plikow->dir();
	delete system_plikow;*/
	//delete system_plikow;
	
	
	//dysk->zapisz_dysk("dysk.txt");
	
	
	/*
	UchwytPliku *plik = system_plikow->otworz_do_zapisu("aaa");
	system_plikow->zapisz_plik(plik, "ascd\nefh\n\nijklfds", 17);
	system_plikow->zamknij_plik(plik);
	
	delete system_plikow;
	system_plikow = new SystemPlikow(dysk);

	plik = system_plikow->otworz_do_odczytu("aaa");
	string s;
	while(czytaj_linie(system_plikow, plik, s))
	{
		cout<<s << endl;
	}
	system_plikow->zamknij_plik(plik);

	*/
	int x;
	do{

	cout << "Menu " << endl;
	cout << "1 - otworz plik z dysku\n2 - utworz plik\n3 - usun plik\n4 - wyswietl katalog\n5 - formatuj dysk\n6 - wyswietl tablice FAT\n7 - wyswietl dysk\n8 - utworz kopie pliku\n9 - zapisz do pliku tekstowego\n0 - wyjscie" << endl;
	//int x;
	//cout << sizeof(UCB) << endl;
	//cout << system_plikow->get_katalog() << endl;
	cout << "Podaj nr wybranej operacji: " << endl;
	cin >> x;
	switch(x)
	{
	case 1:
		{
			string plik;
			cout << "Podaj nazwe pliku: " << endl;
			cin >> plik;
			if (system_plikow->znajdz_plik(plik)!=NULL)
			{
				UchwytPliku *p = system_plikow->otworz_do_odczytu(plik);
				string bufor;
				char tab[100];
				while (p->eof()==false)
				{
					int liczba_przeczytanych=system_plikow->czytaj_plik(p, tab, 30);
					tab[liczba_przeczytanych] = 0;
					bufor+=tab;
				}
				cout << bufor << endl;
				system_plikow->zamknij_plik(p);
				break;
			}
			else
			{
				cout << "Nie ma takiego pliku! " << endl;
				break;
			}
		}
	case 2:
		{
			string nazwa;
			cout << "Podaj nazwe pliku: " << endl;
			cin >> nazwa;
			UchwytPliku *plik = system_plikow->otworz_do_zapisu(nazwa);
			string dane;
			cout << "Wprowadz dane pliku: " << endl;
			cin >> dane;
			system_plikow->zapisz_plik(plik, dane.c_str(), dane.size());

			system_plikow->zamknij_plik(plik);
			break;
		}
	case 3:
		{
			string nazwa;
			cout << "Podaj nazwe pliku, ktory ma zostac usuniety: " << endl;
			cin >> nazwa;
			system_plikow->usun_plik(nazwa);
			break;
		}
	case 4:
		{
			system_plikow->dir();
			break;
		}
	case 5:
		{
			delete system_plikow;
			Dysk *dysk = new Dysk();
			SystemPlikow::formatuj(dysk);
			SystemPlikow *system_plikow = new SystemPlikow(dysk);
		}
	case 6:
		{
			system_plikow->wyswietl_FAT();
			break;
		}
	case 7:
		{
			for (int i=0; i<LICZBA_BLOKOW; i++)
			{
				for(int j=0; j<ROZMIAR_BLOKU; j++)
				{
					cout << system_plikow->m_dysk->get_dysk()[i*ROZMIAR_BLOKU+j] << " ";
				}
				cout << endl;
			}
			break;
		}
	case 8:
		{
			string pl;
			cout << "Podaj nazwe pliku, ktory chcesz skopiowac: " << endl;
			cin >> pl;
			if(system_plikow->znajdz_plik(pl)!=NULL)
			{
				UchwytPliku *p = system_plikow->otworz_do_odczytu(pl);
				string bufor;
				char tab[100];
				while (p->eof()==false)
				{
					int liczba_przeczytanych=system_plikow->czytaj_plik(p, tab, 30);
					tab[liczba_przeczytanych] = 0;
					bufor+=tab;
				}
				system_plikow->zamknij_plik(p);

				string nazwa;
				cout << "Podaj nazwe kopii pliku: " << endl;
				cin >> nazwa;
				UchwytPliku *plik = system_plikow->otworz_do_zapisu(nazwa);
				system_plikow->zapisz_plik(plik, bufor.c_str(), bufor.size());

				system_plikow->zamknij_plik(plik);
			}


			break;
		}
	case 9:
		{
			dysk->zapisz_dysk("dysk.txt");
			break;
		}
	case 0:
		{
			break;
		}
	default:
		{
			cout << "Nie ma takiej opcji" << endl;
		}
	}
	}while(x!=0);

	//delete system_plikow;




	/*PAMIEC p;
	
	string s;
	Dysk*dysk = new Dysk();
	dysk->wczytaj_dysk("dysk.txt");
	SystemPlikow *system_plikow = new SystemPlikow(dysk);
	UchwytPliku *nazwa = system_plikow->otworz_do_odczytu("M");
	
	do
	{
		for (int i = 0; i <= 60; i++)
		{
			czytaj_linie(system_plikow, nazwa, s);
			//cout << s;
			p.element[i] = s;
			cout << p.element[i] << endl;
		}
	} while (s == "$END");*/

	system("PAUSE");
	return 0;
}

