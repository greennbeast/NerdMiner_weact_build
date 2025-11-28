// Multi-threaded micro-benchmark: spawn N worker tasks that run the project's
// double-SHA routine and a reporter that prints aggregate kH/s every reportMs.

#include <Arduino.h>
#include <esp_timer.h>
#include <esp_task_wdt.h>

#include "ShaTests/nerdSHA256.h"
#include "timeconst.h"

#include <stdlib.h>

static volatile uint64_t *s_counts = NULL;
static int s_threads = 0;
static uint64_t s_start_us = 0;

// Worker task: repeatedly run double-SHA and accumulate to per-thread counter
static void bench_worker(void *arg)
{
    int idx = (int)(uintptr_t)arg;

    nerd_sha256 mid;
    uint8_t dataIn[16];
    for (int i = 0; i < 16; ++i) dataIn[i] = (uint8_t)i;
    nerd_midstate(&mid, dataIn, 12);
    uint8_t out[NERD_DIGEST_SIZE];

    uint32_t local = 0;
    while (true) {
        nerd_double_sha2(&mid, dataIn, out);
        ++local;

        // Flush local count more frequently to make reporter see progress
        if ((local & 0xFF) == 0) {
            // atomic add to avoid torn reads
            __atomic_add_fetch((uint64_t*)&s_counts[idx], (uint64_t)local, __ATOMIC_RELAXED);
            local = 0;
            esp_task_wdt_reset();
            taskYIELD();
        }
    }
}

// Reporter task: print aggregate kH/s every reportMs
static void bench_reporter(void *arg)
{
    unsigned long reportMs = (unsigned long)(uintptr_t)arg;
    s_start_us = esp_timer_get_time();

    // Keep a snapshot of last counts to compute per-interval delta
    uint64_t *last_counts = (uint64_t*)calloc(s_threads, sizeof(uint64_t));
    if (!last_counts) {
        Serial.println("Benchmark reporter: failed to allocate last_counts");
        return;
    }

    while (true) {
        vTaskDelay(reportMs / portTICK_PERIOD_MS);
        uint64_t now = esp_timer_get_time();
        uint64_t sum = 0;
        for (int i = 0; i < s_threads; ++i) sum += __atomic_load_n((uint64_t*)&s_counts[i], __ATOMIC_RELAXED);

        double elapsed_s = (now - s_start_us) / 1e6;
        double hs = ((double)sum) / elapsed_s;
        double khs = hs / 1000.0;
        Serial.printf("Benchmark: %llu hashes total -> %.2f kH/s (threads=%d)\n",
                      (unsigned long long)sum, khs, s_threads);

        // Per-thread interval rates
        Serial.print("Per-thread: ");
        for (int i = 0; i < s_threads; ++i) {
            uint64_t cur = __atomic_load_n((uint64_t*)&s_counts[i], __ATOMIC_RELAXED);
            uint64_t delta = cur - last_counts[i];
            double rate_khs = (double)delta / ((double)reportMs / 1000.0) / 1000.0; // kH/s
            Serial.printf("t%d: %.2f kH/s ", i, rate_khs);
            last_counts[i] = cur;
        }
        Serial.println();
    }
}

// Entry: spawn workers + reporter and return (non-blocking)
void runBenchmarkMode(unsigned long reportMs)
{
    if (s_counts) return; // already running

#ifdef BENCH_THREADS
    s_threads = BENCH_THREADS;
#else
    s_threads = MINER_WORKER_COUNT;
#endif

    if (s_threads <= 0) s_threads = 1;

    s_counts = (volatile uint64_t *)calloc(s_threads, sizeof(uint64_t));
    if (!s_counts) {
        Serial.println("Benchmark: failed to allocate counters");
        return;
    }

    Serial.println("\n*** NerdMiner Multi-threaded Micro-benchmark Mode ***");
    Serial.printf("Spawning %d worker(s). Reporting every %lu ms\n", s_threads, reportMs);

    int cores = (SOC_CPU_CORES_NUM >= 2) ? 2 : 1;
    for (int i = 0; i < s_threads; ++i) {
        char tname[16];
        snprintf(tname, sizeof(tname), "Bench-%d", i);
        TaskHandle_t th = NULL;
        xTaskCreatePinnedToCore(bench_worker, tname, 4096, (void*)(uintptr_t)i, 2, &th, i % cores);
        if (th) esp_task_wdt_add(th);
    }

    TaskHandle_t rth = NULL;
    xTaskCreatePinnedToCore(bench_reporter, "BenchRep", 4096, (void*)(uintptr_t)reportMs, 2, &rth, 0);
    if (rth) esp_task_wdt_add(rth);
}

