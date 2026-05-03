# Soal 1 — Present Day, Present Time

**Sistem Operasi 2026**

| | |
|---|---|
| **Nama** | A. Algifari Rantiga Isdar |
| **NRP** | 5027251112 |

---

## Deskripsi Program

Program ini merupakan implementasi sistem komunikasi client-server berbasis socket TCP menggunakan bahasa C.

Sistem ini mensimulasikan jaringan komunikasi bernama **The Wired**, di mana beberapa client (NAVI) dapat terhubung ke server pusat, saling bertukar pesan melalui mekanisme broadcast, serta menyediakan akses administratif khusus untuk **The Knights**.

Program terdiri dari dua komponen utama:

| Komponen | File |
|---|---|
| Server | `wired.c` |
| Client | `navi.c` |

---

## Fitur yang Diimplementasikan

### 1. Koneksi Client ke Server

Client dapat terhubung ke server menggunakan alamat IP dan port yang didefinisikan pada file `protocol.h`.

Client menggunakan:
```c
socket()
connect()
```

Server menggunakan:
```c
bind()
listen()
accept()
```

---

### 2. Asynchronous I/O tanpa Fork

Client dapat secara bersamaan menerima pesan dari server dan mengirim input dari user, tanpa menggunakan `fork()`.

Menggunakan `select()` untuk memonitor stdin dan socket secara bersamaan.

---

### 3. Multi-client Handling

Server mampu menangani banyak client secara bersamaan menggunakan `select()` untuk memantau semua file descriptor — ringan, efisien, tanpa thread/fork.

---

### 4. Identitas Unik

Setiap client wajib memiliki nama unik. Server melakukan pengecekan via `is_name_taken()`. Jika nama sudah dipakai:

```
[System] Name already used
```

---

### 5. Broadcast Message

Setiap pesan yang dikirim client akan diteruskan ke seluruh client lain yang aktif melalui fungsi `broadcast()`.

---

### 6. Command `/exit`

Client dapat keluar dengan command `/exit`. Output:

```
[System] Disconnecting from The Wired...
```

Server akan menutup socket dan menghapus client dari daftar aktif.

---

### 7. Admin Authentication (The Knights)

Nama khusus `The Knights` akan meminta password. Password yang digunakan: `protocol7`.

Jika benar:
```
[System] Authentication Successful. Granted Admin privileges.
```

---

### 8. RPC Admin Console

Admin memiliki akses ke menu berikut:

| Opsi | Fungsi |
|---|---|
| 1 | Check Active Entities — menampilkan jumlah user aktif (admin tidak dihitung) |
| 2 | Check Server Uptime — menampilkan lama server berjalan |
| 3 | Emergency Shutdown — mematikan server |
| 4 | Disconnect — keluar dari mode admin |

---

### 9. Ctrl+C Handling

Saat admin menekan `Ctrl+C`, sistem akan memperlakukannya sama seperti opsi **4. Disconnect** melalui signal handler.

---

### 10. Logging

Semua aktivitas dicatat pada `history.log` dengan format:

```
[YYYY-MM-DD HH:MM:SS] [TYPE] [MESSAGE]
```

Contoh:
```
[2026-04-26 19:06:46] [System] [User 'alice' connected]
```

---

## Penjelasan Kode

### Struct Client

Menyimpan data tiap client dengan field:

| Field | Keterangan |
|---|---|
| `fd` | File descriptor socket |
| `name` | Nama client |
| `logged_in` | Status login |
| `is_admin` | Penanda admin |
| `waiting_password` | Status menunggu password |

### Fungsi-fungsi Utama

| Fungsi | Keterangan |
|---|---|
| `reset_client()` | Mereset seluruh data client saat disconnect |
| `trim()` | Menghapus karakter newline dari input |
| `log_event()` | Mencatat event ke file log |
| `is_name_taken()` | Mengecek apakah nama sudah digunakan |
| `broadcast()` | Mengirim pesan ke seluruh client aktif |
| `send_menu()` | Menampilkan menu admin |

---

## Edge Case Handling

| # | Kasus | Penanganan |
|---|---|---|
| 1 | Server belum aktif | Output: `Connection Failed` |
| 2 | Nama kosong | Output: `[System] Name cannot be empty` |
| 3 | Duplicate username | Nama yang sudah dipakai ditolak |
| 4 | Disconnect mendadak | Server otomatis membersihkan socket dan state client |
| 5 | `/exit` tidak menampilkan pesan | Sinkronisasi client-server sebelum socket ditutup |
| 6 | Ctrl+C | Di-handle dengan signal handler agar disconnect tetap graceful |
| 7 | Buffer overflow | Menggunakan `snprintf()` untuk membatasi ukuran output |
| 8 | Logging tidak rapi | Pesan log dipisahkan dari pesan broadcast |

---

## Cara Menjalankan

### Compile

```bash
# Server
gcc wired.c -o wired

# Client
gcc navi.c -o navi
```

### Jalankan Server

```bash
./wired
```

### Jalankan Client

```bash
./navi
```

---

## Testing

### Login User Biasa

```
Enter your name: alice
--- Welcome to The Wired, alice ---
```

### Duplicate User

```
[System] Name already used
```

### Admin Login

```
Enter your name: The Knights
Enter Password: protocol7
```

### Broadcast Test

Client 1 mengirim:
```
hello
```

Client 2 menerima:
```
[alice]: hello
```

---

## Kendala yang Dihadapi

| # | Kendala | Solusi |
|---|---|---|
| 1 | Disconnect message tidak muncul (client keluar terlalu cepat) | Sinkronisasi sebelum terminate |
| 2 | Ctrl+C langsung menutup program | Signal handler |
| 3 | Log berantakan | Pisahkan buffer log dan buffer broadcast |

---

## Kesimpulan

Program berhasil mengimplementasikan seluruh requirement:

- ✅ Koneksi client-server
- ✅ Asynchronous I/O
- ✅ Multi-client handling
- ✅ Unique username
- ✅ Broadcast
- ✅ Admin RPC
- ✅ Logging
- ✅ Graceful disconnect
- ✅ Ctrl+C handling
