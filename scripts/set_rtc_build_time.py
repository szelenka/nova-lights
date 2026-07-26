from datetime import datetime

Import("env")

now = datetime.now()
env.Append(
    CPPDEFINES=[
        ("NOVA_RTC_YEAR", now.year),
        ("NOVA_RTC_MONTH", now.month),
        ("NOVA_RTC_DAY", now.day),
        ("NOVA_RTC_HOUR", now.hour),
        ("NOVA_RTC_MINUTE", now.minute),
        ("NOVA_RTC_SECOND", now.second),
    ]
)
