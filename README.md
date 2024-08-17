# img2datetime

Rename IMG to the created time.

## Usage

```bash
git clone https://github.com/liu-congcong/img2datetime.git
cd img2datetime
gcc -o img2datetime img2datetime.c
```

```bash
img2datetime -h
Rename photos using created time v0.1 (https://github.com/liu-congcong/img2datetime)
Usage:
    ./img2datetime --input <str>.
Options:
    -i/--input: the directory containing the photos.
```

## Change logs

* 0.1.0 (20240817): 暂时仅支持jpeg|jpg格式的图片,下一个版本增加heic.
