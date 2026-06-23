#include <iostream>
#include <sqlite3.h> // Library database SQLite3

using namespace std;
#define No int

const int MAX = 5;
int antrian[MAX];
int depan = -1;
int belakang = -1;
string namaAntrian[MAX];

sqlite3* db;

bool isFull();
bool isEmpty();
void tampilkanAntrian();

void resetQueue() {
    depan = -1;
    belakang = -1;
}

// Fungsi untuk eksekusi perintah SQL secara ringkas
void jalankanSQL(const char* sql) {
    if (db == nullptr) {
        cout << "Database belum siap. Operasi dibatalkan." << endl;
        return;
    }

    char* errorMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errorMsg);
    if (rc != SQLITE_OK) {
        cout << "Error SQL: " << errorMsg << endl;
        sqlite3_free(errorMsg);
    }
}

// Inisialisasi database dan buat tabel jika belum ada
bool inisialisasiDatabase() {
    int rc = sqlite3_open("antrian.db", &db);
    if (rc) {
        cout << "Gagal membuka database: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        db = nullptr;
        return false;
    }

    // Buat tabel untuk mencatat riwayat semua antrian yang masuk
    const char* sql = "CREATE TABLE IF NOT EXISTS riwayat_antrian ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "nomor_antrian INTEGER, "
                      "nama_pasien TEXT, "
                      "status TEXT);";
    jalankanSQL(sql);
    return true;
}
void ClearQueue() {
    resetQueue();
    const char* sql = "DELETE FROM riwayat_antrian WHERE status = 'Menunggu';";
    jalankanSQL(sql);
    cout << "Antrian telah dikosongkan." << endl;

}

void loadAntrianDariDatabase() {
    resetQueue();

    const char* sql = "SELECT nomor_antrian, nama_pasien "
                      "FROM riwayat_antrian "
                      "WHERE status = 'Menunggu' "
                      "ORDER BY id ASC;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cout << "Gagal memuat antrian dari database: " << sqlite3_errmsg(db) << endl;
        return;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (isFull()) {
            cout << "Antrian tersimpan di database melebihi kapasitas memori program."
                 << endl;
            break;
        }

        int nomor = sqlite3_column_int(stmt, 0);
        const unsigned char* namaText = sqlite3_column_text(stmt, 1);
        string namaPasien = namaText ? reinterpret_cast<const char*>(namaText) : "";

        if (depan == -1) {
            depan = 0;
        }
        belakang = (belakang + 1) % MAX;
        antrian[belakang] = nomor;
        namaAntrian[belakang] = namaPasien;
    }

    sqlite3_finalize(stmt);

    cout << "Data antrian dari database berhasil dimuat." << endl;
    tampilkanAntrian();
}

bool isFull() {
    return ((depan == 0 && belakang == MAX - 1) || (depan == belakang + 1));
}

bool isEmpty() {
    return (depan == -1);
}

// Menambah antrian dan simpan ke database
void enqueue(No elemen,string namaPasien) {
    if (isFull()) {
        cout << "Antrian penuh! Gagal menambahkan " << elemen << endl;
    } else {
        if (depan == -1) {
            depan = 0;
        }
        belakang = (belakang + 1) % MAX;
        antrian[belakang] = elemen;
        namaAntrian[belakang] = namaPasien;
        
        cout << "Nomor " << elemen << " berhasil masuk antrian." << endl;

        // INSERT data ke database sqlite
        string sqlQuery = "INSERT INTO riwayat_antrian (nomor_antrian, nama_pasien, status) VALUES (" 
                          + to_string(elemen) + ", '" + (namaPasien) + "', 'Menunggu');";
        jalankanSQL(sqlQuery.c_str());
    }
}

// Memanggil antrian dan memperbarui status di database
void dequeue() {
    if (isEmpty()) {
        cout << "Antrian kosong! Tidak ada yang bisa dipanggil." << endl;
    } else {
        int nomorDipanggil = antrian[depan];
        cout << "Nomor " << nomorDipanggil << " dipanggil silakan maju." << endl;

        // UPDATE status di database sqlite
        string sqlQuery = "UPDATE riwayat_antrian SET status = 'Selesai' WHERE nomor_antrian = " 
                          + to_string(nomorDipanggil) + " AND status = 'Menunggu';";
        jalankanSQL(sqlQuery.c_str());

        if (depan == belakang) {
            depan = -1;
            belakang = -1;
        } else {
            depan = (depan + 1) % MAX;
        }
    }
}

void tampilkanAntrian() {
    if (isEmpty()) {
        cout << "Antrian saat ini: [ Kosong ]" << endl;
    } else {
        cout << "Antrian saat ini: " << endl;
        int i = depan;
        while (true) {
            cout << "[" << antrian[i] << "] " << namaAntrian[i] << endl;
            if (i == belakang) break;
            i = (i + 1) % MAX;
        }
        cout << endl;
    }
}

int main() {
    if (!inisialisasiDatabase()) {
        return 1;
    }

    loadAntrianDariDatabase();

    No pilihan, data;
    string namaPasien;
    
    do {
        cout << "\n=== MENU ANTRIAN + DATABASE ===" << endl;
        cout << "1. Tambah Antrian (Enqueue)" << endl;
        cout << "2. Panggil Antrian (Dequeue)" << endl;
        cout << "3. Lihat Antrian" << endl;
        cout << "4. Kosongkan Antrian" << endl;
        cout << "5. Keluar" << endl;
        cout << "Pilih menu (1-5): ";
        cin >> pilihan;
        
        switch (pilihan) {
            case 1:
                cout << "Masukkan nomor antrian (angka): ";
                cin >> data;
                cin.ignore(); // Membersihkan buffer input sebelum getline
                cout << "Masukkan nama pasien: ";
                getline(cin, namaPasien); // Input nama pasien
                enqueue(data, namaPasien);
                break;
            case 2:
                dequeue();
                break;
            case 3:
                tampilkanAntrian();
                break;
            case 4:
                ClearQueue();
                break;
            case 5:
                cout << "Program selesai." << endl;
                break;
            default:
                cout << "Pilihan tidak valid!" << endl;
        }
    } while (pilihan != 5);
    
    if (db != nullptr) {
        sqlite3_close(db); // Tutup koneksi database sebelum keluar program
    }
    return 0;
}
