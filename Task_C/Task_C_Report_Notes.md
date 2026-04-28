# Task C Report Notes - Merge Sort using C++ std::thread

## 1. Хийсэн ажил

Энэ хэсэгт Merge Sort алгоритмыг C++ стандарт сангийн `std::thread` ашиглан олон thread-тэй хувилбараар хэрэгжүүлсэн. Харьцуулалт хийхийн тулд ижил өгөгдөл дээр дараалсан Merge Sort хувилбарыг мөн ажиллуулж, CPU дээрх multithreading хувилбарын гүйцэтгэлийг хэмжсэн.

Туршилтын өгөгдлийн хэмжээ:

- 10,000 элемент
- 100,000 элемент
- 1,000,000 элемент

Хэмжсэн үзүүлэлтүүд:

- Execution time
- Computation time
- SpeedUp
- Total operations буюу ойролцоогоор `n log2(n)`
- Achievable performance
- Correctness буюу эрэмбэлэлт зөв болсон эсэх

`std::thread` хувилбар нь CPU-only тул GPU рүү өгөгдөл дамжуулахгүй. Иймээс `Data transfer time = 0`, `Amount of data transferred = 0` гэж тэмдэглэсэн.

## 2. Онолын ойлголт

Merge Sort нь divide-and-conquer зарчимтай эрэмбэлэлтийн алгоритм юм. Эхлээд массивыг хоёр хэсэгт хувааж, хэсэг бүрийг тус тусад нь эрэмбэлнэ. Дараа нь хоёр эрэмбэлэгдсэн хэсгийг нэгтгэж, том эрэмбэлэгдсэн массив үүсгэнэ.

Дараалсан хувилбарт зүүн хэсэг дууссаны дараа баруун хэсгийг эрэмбэлдэг. Харин multithreading хувилбарт зүүн болон баруун хэсгүүдийг боломжтой үед зэрэг ажиллуулж болно. Ингэснээр CPU-ийн олон цөмийг ашиглаж нийт ажиллах хугацааг багасгах боломжтой.

## 3. Thread-ийн зохион байгуулалт

Энэ хэрэгжүүлэлтэд `std::thread` ашиглан массивыг хэд хэдэн chunk болгон хувааж, chunk бүрийг тусдаа thread дээр Merge Sort ашиглан эрэмбэлсэн. Дараа нь эрэмбэлэгдсэн хэсгүүдийг үе шаттайгаар нэгтгэсэн.

Ажиллагааны санаа:

1. Массивыг thread-ийн тоотой ойролцоо хэд хэдэн chunk болгон хуваана.
2. Chunk бүрийг тусдаа `std::thread` дээр дараалсан Merge Sort-оор эрэмбэлнэ.
3. Бүх sort thread дээр `join()` хийж, бүх хэсэг эрэмбэлэгдэж дуусахыг хүлээнэ.
4. Хөрш sorted chunk-үүдийг merge thread-үүдээр зэрэг нэгтгэнэ.
5. Merge pass бүрийн дараа дахин `join()` хийж дараагийн merge түвшин рүү орно.

`join()` нь synchronization point юм. Өөрөөр хэлбэл бүх thread тухайн түвшний ажлаа бүрэн дуусгасны дараа л дараагийн merge түвшин эхэлнэ.

Mutex ашиглаагүй шалтгаан:

- Thread бүр массивын өөр өөр, давхцахгүй index range дээр ажилладаг.
- Зүүн болон баруун хэсэг нэг санах ойн ижил index дээр зэрэг бичихгүй.
- Дараагийн merge түвшин нь өмнөх түвшний thread-үүд дууссаны дараа хийгддэг.

Иймээс энэ тохиолдолд `join()` synchronization хангалттай.

## 4. Псевдо код

### Sequential Merge Sort

```text
SEQUENTIAL_MERGE_SORT(A, left, right):
    if right - left <= 1:
        return

    mid = (left + right) / 2

    SEQUENTIAL_MERGE_SORT(A, left, mid)
    SEQUENTIAL_MERGE_SORT(A, mid, right)

    MERGE(A, left, mid, right)
```

### std::thread Merge Sort

```text
THREADED_MERGE_SORT(A, thread_count):
    chunks = SPLIT_ARRAY(A, thread_count)

    for each chunk in chunks:
        create thread to run SEQUENTIAL_MERGE_SORT(chunk)

    join all sort threads

    runs = chunks
    while number of runs > 1:
        for each neighboring pair of runs:
            create thread to MERGE(left_run, right_run)

        join all merge threads
        runs = merged runs
```

### Merge operation

```text
MERGE(A, left, mid, right):
    i = left
    j = mid
    k = left

    while i < mid and j < right:
        if A[i] <= A[j]:
            TEMP[k] = A[i]
            i = i + 1
        else:
            TEMP[k] = A[j]
            j = j + 1
        k = k + 1

    copy remaining left elements
    copy remaining right elements
    copy TEMP[left:right] back to A[left:right]
```

## 5. Санах ой, кэш, CPU түвшний тайлбар

Merge Sort нь нэмэлт temporary array ашигладаг. Энэ temporary array нь merge хийх үед хоёр sorted хэсгийг нэгтгэхэд хэрэглэгдэнэ. Том массив дээр merge хийх үед санах ойгоос дараалсан байдлаар унших, бичих үйлдэл их гардаг тул cache locality харьцангуй сайн байдаг.

Multithreading хувилбарт CPU-ийн олон цөм зэрэг ажиллаж, массивын өөр өөр хэсгүүдийг эрэмбэлнэ. Энэ нь том хэмжээтэй өгөгдөл дээр ашигтай. Харин жижиг хэмжээтэй өгөгдөл дээр thread үүсгэх overhead нь ашиг өгөхгүй байж болно. Тиймээс кодонд `min_parallel_size` босго ашигласан. Хэт жижиг хэсэг дээр шинэ thread үүсгэхгүй, дараалсан Merge Sort ашигладаг.

## 6. Үр дүнгийн тайлбар дээр бичиж болох санаа

- 10k хэмжээтэй өгөгдөл дээр speedup бага эсвэл бүр удаан байж болно. Учир нь thread үүсгэх, join хийх overhead нь бодит sort хийх ажлаас их нөлөөлж болно.
- 100k болон 1M хэмжээтэй өгөгдөл дээр multithreading хувилбар илүү үр дүнтэй байх магадлалтай.
- SpeedUp нь CPU-ийн цөмийн тоо, OS scheduling, санах ойн bandwidth, thread count-оос хамаарна.
- Хэт олон thread тохируулбал заавал хурдан болохгүй. Thread удирдлагын overhead болон memory bandwidth-ийн хязгаарлалтаас болж гүйцэтгэл буурч болно.

## 7. Тайланд оруулах богино дүгнэлтийн жишээ

C++ `std::thread` ашигласан Merge Sort хувилбар нь алгоритмын divide-and-conquer бүтэцтэй сайн зохицож байна. Том хэмжээний өгөгдөл дээр массивын хоёр хэсгийг зэрэг эрэмбэлснээр execution time буурах боломжтой. Гэхдээ жижиг өгөгдөл дээр thread үүсгэх overhead нөлөөлөх тул parallel хувилбар үргэлж хурдан байх албагүй. Иймээс multithreading Merge Sort нь их хэмжээний өгөгдөлтэй үед илүү тохиромжтой гэж дүгнэж болно.
