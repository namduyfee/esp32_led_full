# Có 2 task chính là : task_make_led_signal và task_make_audio_signal

* task_make_led_signal:

nhiệm vụ : chỉ tiếp nhận buffer và tạo xung để điều khiển led.
giao tiếp với task này thông qua queue xLedRequestList. mỗi phần tử của queue là 1 struct request_strip_led_t.


typedef struct {
    struct {
        void *data;             /**< lưu địa chỉ sau khi malloc*/
        uint32_t tot_byte;      /**< tổng số byte của hiệu ứng>
    } channel0;

    struct {
        void *data;             /**< lưu địa chỉ sau khi malloc*/
        uint32_t tot_byte;      /**< tổng số byte của hiệu ứng>
    } channel1;

} request_strip_led_t;

channel0.data , channel1.data sẽ được free trong task task_make_led_signal.
kênh nào không có update mới thì gán .data = NULL. 

NOTE: rmt sử dụng mảng để thực hiện việc xuất tín hiệu.
 

* task_make_audio_signal

nhiệm vụ : chỉ nhận buffer và tạo xung audio một lần duy nhất.
giao tiếp với task này thông qua queue xAudioBufferList. mối phần tử queue là 1 struct audio_buf_t. 

typedef struct {

    void *data;                 /**< lưu địa chỉ sau khi malloc>
    uint32_t tot_byte;          /**< tổng số byte của buffer>

} audio_buf_t;

.data sẽ được free trong task task_make_led_signal
