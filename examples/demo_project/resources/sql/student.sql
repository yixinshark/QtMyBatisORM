<?xml version="1.0" encoding="UTF-8"?>
<mapper namespace="Student">
    <!-- 基础查询 -->
    <select id="findAll" resultType="QVariantMap">
        SELECT id, student_number, name, gender, birth_date, major, grade, 
               phone, email, address, enrollment_date, status, created_at, updated_at
        FROM students 
        WHERE status = 'ACTIVE' 
        ORDER BY student_number
    </select>

    <select id="findById" resultType="QVariantMap">
        SELECT id, student_number, name, gender, birth_date, major, grade, 
               phone, email, address, enrollment_date, status, created_at, updated_at
        FROM students 
        WHERE id = :arg1
    </select>

    <select id="findByStudentNumber" resultType="QVariantMap">
        SELECT id, student_number, name, gender, birth_date, major, grade, 
               phone, email, address, enrollment_date, status, created_at, updated_at
        FROM students 
        WHERE student_number = :arg1
    </select>

    <select id="existsByStudentNumber" resultType="int">
        SELECT COUNT(*) FROM students WHERE student_number = :arg1
    </select>

    <select id="findByNamePattern" resultType="QVariantMap">
        SELECT id, student_number, name, gender, birth_date, major, grade, 
               phone, email, address, enrollment_date, status, created_at, updated_at
        FROM students 
        WHERE name LIKE :arg1 AND status = 'ACTIVE'
        ORDER BY student_number
    </select>

    <select id="findByMajor" resultType="QVariantMap">
        SELECT id, student_number, name, gender, birth_date, major, grade, 
               phone, email, address, enrollment_date, status, created_at, updated_at
        FROM students 
        WHERE major LIKE :arg1 AND status = 'ACTIVE'
        ORDER BY student_number
    </select>

    <select id="count" resultType="int">
        SELECT COUNT(*) FROM students WHERE status = 'ACTIVE'
    </select>

    <!-- 高级查询 -->
    <select id="findWithPagination" resultType="QVariantMap">
        SELECT id, student_number, name, gender, birth_date, major, grade, 
               phone, email, address, enrollment_date, status, created_at, updated_at
        FROM students 
        WHERE status = 'ACTIVE'
        ORDER BY student_number
        LIMIT :arg1 OFFSET :arg2
    </select>

    <select id="findByMajorWithPagination" resultType="QVariantMap">
        SELECT id, student_number, name, gender, birth_date, major, grade, 
               phone, email, address, enrollment_date, status, created_at, updated_at
        FROM students 
        WHERE major LIKE :arg1 AND status = 'ACTIVE'
        ORDER BY student_number
        LIMIT :arg2 OFFSET :arg3
    </select>

    <select id="findByGrade" resultType="QVariantMap">
        SELECT id, student_number, name, gender, birth_date, major, grade, 
               phone, email, address, enrollment_date, status, created_at, updated_at
        FROM students 
        WHERE grade = :arg1 AND status = 'ACTIVE'
        ORDER BY student_number
    </select>

    <select id="findStudentsWithCourseCount" resultType="QVariantMap">
        SELECT s.*, COUNT(e.id) as course_count
        FROM students s
        LEFT JOIN enrollments e ON s.id = e.student_id
        WHERE s.status = 'ACTIVE'
        GROUP BY s.id
        ORDER BY course_count DESC, s.student_number
    </select>

    <select id="findStudentsInCourse" resultType="QVariantMap">
        SELECT s.*
        FROM students s
        JOIN enrollments e ON s.id = e.student_id
        WHERE e.course_id = :arg1 AND s.status = 'ACTIVE'
        ORDER BY s.student_number
    </select>

    <select id="findTopStudentsByGPA" resultType="QVariantMap">
        SELECT s.*, AVG(e.grade) as gpa
        FROM students s
        JOIN enrollments e ON s.id = e.student_id
        WHERE e.grade IS NOT NULL AND e.status = 'COMPLETED' AND s.status = 'ACTIVE'
        GROUP BY s.id
        HAVING AVG(e.grade) >= :arg1
        ORDER BY gpa DESC
        LIMIT :arg2
    </select>

    <!-- 统计查询 -->
    <select id="countByMajor" resultType="QVariantMap">
        SELECT major, COUNT(*) as count
        FROM students
        WHERE status = 'ACTIVE'
        GROUP BY major
        ORDER BY count DESC
    </select>

    <select id="countByGrade" resultType="QVariantMap">
        SELECT grade, COUNT(*) as count
        FROM students
        WHERE status = 'ACTIVE'
        GROUP BY grade
        ORDER BY grade
    </select>

    <select id="getAverageGradeByMajor" resultType="QVariantMap">
        SELECT s.major, AVG(e.grade) as average_grade
        FROM students s
        JOIN enrollments e ON s.id = e.student_id
        WHERE e.grade IS NOT NULL AND e.status = 'COMPLETED' AND s.status = 'ACTIVE'
        GROUP BY s.major
        ORDER BY average_grade DESC
    </select>

    <!-- 验证查询 -->
    <select id="existsByEmail" resultType="int">
        SELECT COUNT(*) FROM students WHERE email = :arg1
    </select>

    <select id="checkEnrollmentEligibility" resultType="int">
        SELECT COUNT(*) FROM enrollments e 
        JOIN students s ON e.student_id = s.id 
        WHERE s.student_number = :arg1
    </select>

    <!-- 插入操作 -->
    <insert id="insert">
        INSERT INTO students (student_number, name, gender, birth_date, major, grade, 
                            phone, email, address, enrollment_date, status)
        VALUES (:student_number, :name, :gender, :birth_date, :major, :grade, 
                :phone, :email, :address, :enrollment_date, :status)
    </insert>

    <!-- 更新操作 -->
    <update id="update">
        UPDATE students 
        SET name = :name, gender = :gender, birth_date = :birth_date, 
            major = :major, grade = :grade, phone = :phone, email = :email, 
            address = :address, updated_at = CURRENT_TIMESTAMP
        WHERE id = :id
    </update>

    <update id="updateByStudentNumber">
        UPDATE students 
        SET name = :name, gender = :gender, birth_date = :birth_date, 
            major = :major, grade = :grade, phone = :phone, email = :email, 
            address = :address, updated_at = CURRENT_TIMESTAMP
        WHERE student_number = :student_number
    </update>

    <!-- 删除操作 -->
    <delete id="deleteById">
        DELETE FROM students WHERE id = :arg1
    </delete>

    <update id="softDeleteById">
        UPDATE students SET status = 'INACTIVE', updated_at = CURRENT_TIMESTAMP 
        WHERE id = :arg1
    </update>
</mapper> 