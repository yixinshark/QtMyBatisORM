<?xml version="1.0" encoding="UTF-8"?>
<mapper namespace="Init">
    <!-- 创建学生表 -->
    <update id="createStudentTable">
        CREATE TABLE IF NOT EXISTS `students` (
            `id` INTEGER PRIMARY KEY AUTOINCREMENT,
            `student_number` VARCHAR(20) UNIQUE NOT NULL,
            `name` VARCHAR(100) NOT NULL,
            `gender` VARCHAR(10) DEFAULT NULL,
            `birth_date` DATE DEFAULT NULL,
            `major` VARCHAR(100) DEFAULT NULL,
            `grade` INTEGER DEFAULT NULL,
            `phone` VARCHAR(20) DEFAULT NULL,
            `email` VARCHAR(255) DEFAULT NULL,
            `address` TEXT DEFAULT NULL,
            `enrollment_date` DATE DEFAULT NULL,
            `status` VARCHAR(20) DEFAULT 'ACTIVE',
            `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
            `updated_at` DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    </update>

    <!-- 创建课程表 -->
    <update id="createCourseTable">
        CREATE TABLE IF NOT EXISTS `courses` (
            `id` INTEGER PRIMARY KEY AUTOINCREMENT,
            `course_code` VARCHAR(20) UNIQUE NOT NULL,
            `name` VARCHAR(200) NOT NULL,
            `description` TEXT DEFAULT NULL,
            `credits` INTEGER DEFAULT 0,
            `hours` INTEGER DEFAULT 0,
            `teacher` VARCHAR(100) DEFAULT NULL,
            `department` VARCHAR(100) DEFAULT NULL,
            `semester` VARCHAR(20) DEFAULT NULL,
            `max_students` INTEGER DEFAULT 50,
            `status` VARCHAR(20) DEFAULT 'ACTIVE',
            `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
            `updated_at` DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    </update>

    <!-- 创建选课表 -->
    <update id="createEnrollmentTable">
        CREATE TABLE IF NOT EXISTS `enrollments` (
            `id` INTEGER PRIMARY KEY AUTOINCREMENT,
            `student_id` INTEGER NOT NULL,
            `course_id` INTEGER NOT NULL,
            `enrollment_date` DATE DEFAULT NULL,
            `grade` DECIMAL(5,2) DEFAULT NULL,
            `status` VARCHAR(20) DEFAULT 'ENROLLED',
            `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
            `updated_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (`student_id`) REFERENCES `students`(`id`) ON DELETE CASCADE,
            FOREIGN KEY (`course_id`) REFERENCES `courses`(`id`) ON DELETE CASCADE,
            UNIQUE(`student_id`, `course_id`)
        )
    </update>

    <!-- 创建索引 -->
    <update id="createIndexStudentsNumber">
        CREATE INDEX IF NOT EXISTS `idx_students_number` ON `students`(`student_number`)
    </update>
    
    <update id="createIndexStudentsMajor">
        CREATE INDEX IF NOT EXISTS `idx_students_major` ON `students`(`major`)
    </update>
    
    <update id="createIndexCoursesCode">
        CREATE INDEX IF NOT EXISTS `idx_courses_code` ON `courses`(`course_code`)
    </update>
    
    <update id="createIndexCoursesDepartment">
        CREATE INDEX IF NOT EXISTS `idx_courses_department` ON `courses`(`department`)
    </update>
    
    <update id="createIndexEnrollmentsStudent">
        CREATE INDEX IF NOT EXISTS `idx_enrollments_student` ON `enrollments`(`student_id`)
    </update>
    
    <update id="createIndexEnrollmentsCourse">
        CREATE INDEX IF NOT EXISTS `idx_enrollments_course` ON `enrollments`(`course_id`)
    </update>

    <!-- 插入示例数据 -->
    <update id="insertSampleStudents">
        INSERT OR IGNORE INTO students (student_number, name, gender, birth_date, major, grade, phone, email, enrollment_date) 
        VALUES 
            ('2021001', '张三', '男', '2003-05-15', '计算机科学与技术', 2021, '13800138001', 'zhangsan@edu.cn', '2021-09-01'),
            ('2021002', '李四', '女', '2003-08-20', '软件工程', 2021, '13800138002', 'lisi@edu.cn', '2021-09-01'),
            ('2021003', '王五', '男', '2003-02-10', '数据科学与大数据技术', 2021, '13800138003', 'wangwu@edu.cn', '2021-09-01'),
            ('2022001', '赵六', '女', '2004-11-05', '计算机科学与技术', 2022, '13800138004', 'zhaoliu@edu.cn', '2022-09-01'),
            ('2022002', '钱七', '男', '2004-07-12', '人工智能', 2022, '13800138005', 'qianqi@edu.cn', '2022-09-01')
    </update>
    
    <update id="insertSampleCourses">
        INSERT OR IGNORE INTO courses (course_code, name, description, credits, hours, teacher, department, semester) 
        VALUES 
            ('CS101', '程序设计基础', 'C++编程基础课程', 4, 64, '李老师', '计算机学院', '2023-1'),
            ('CS102', '数据结构与算法', '数据结构和算法设计与分析', 4, 64, '王老师', '计算机学院', '2023-1'),
            ('CS201', '数据库系统', '数据库原理与应用开发', 3, 48, '张老师', '计算机学院', '2023-2'),
            ('CS202', '计算机网络', '计算机网络原理与协议', 3, 48, '刘老师', '计算机学院', '2023-2'),
            ('CS301', '软件工程', '软件开发方法学与项目管理', 3, 48, '陈老师', '计算机学院', '2023-1')
    </update>
    
    <update id="insertSampleEnrollments">
        INSERT OR IGNORE INTO enrollments (student_id, course_id, enrollment_date, grade, status)
        VALUES 
            (1, 1, '2023-09-01', 85.5, 'COMPLETED'),
            (1, 2, '2023-09-01', 78.0, 'COMPLETED'),
            (1, 3, '2024-02-20', NULL, 'ENROLLED'),
            (2, 1, '2023-09-01', 92.0, 'COMPLETED'),
            (2, 2, '2023-09-01', 88.5, 'COMPLETED'),
            (2, 4, '2024-02-20', NULL, 'ENROLLED'),
            (3, 1, '2023-09-01', 76.0, 'COMPLETED'),
            (3, 5, '2023-09-01', 82.5, 'COMPLETED'),
            (4, 1, '2024-02-20', NULL, 'ENROLLED'),
            (5, 1, '2024-02-20', NULL, 'ENROLLED')
    </update>
</mapper> 